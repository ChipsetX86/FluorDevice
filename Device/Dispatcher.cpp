#include "Dispatcher.h"

#include <QtSerialPort/QSerialPort>
#include <QTimer>
#include <QElapsedTimer>
#include <QThread>

#include <NpToolbox/Invoker.h>

#include "DeviceLogging.h"

using namespace Nauchpribor;

struct Dispatcher::PImpl
{
    QSerialPort *port;
    Params params;
    QString errorStr;

    QMutex ioMutex;

    QTimer *writeTimeoutTimer;
    QElapsedTimer writeDelayTimer;
    QTimer *processReadBufferTimer;

    QByteArray readBuffer;

    QMutex accessorsMutex;
    QMap<quint8, Dispatcher::Accessor *> accessors;

    bool setupRtsAndDtr(bool rts, bool dtr, quint32 msecAfter = 0);

    void startWriteTimeoutTimer();
    void stopWriteTimeoutTimer();

    void startProcessReadBufferTimer();
    void stopProcessReadBufferTimer();

    void clearAccessorsReadBuffer();
};

namespace {
    class DispatcherWriteWatcher : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY(DispatcherWriteWatcher)
    public:
        explicit DispatcherWriteWatcher(Dispatcher *dispatcher, QObject *parent = nullptr);
        bool wait();
    public slots:
        void onWriteSuccess();
        void onErrorOccurred();
    private:
        enum Result {
            ResultUnknown,
            ResultSuccess,
            ResultError
        };

        Result m_result;
        QMutex m_mutex;
        QWaitCondition m_waitCondition;
    };
}

Dispatcher::Dispatcher(QObject *parent) : QObject(parent),
    m_pimpl(new PImpl)
{
    m_pimpl->port = new QSerialPort(this);
    m_pimpl->port->setDataBits(QSerialPort::Data8);
    m_pimpl->port->setFlowControl(QSerialPort::NoFlowControl);
    m_pimpl->port->setParity(QSerialPort::NoParity);
    m_pimpl->port->setStopBits(QSerialPort::OneStop);

    m_pimpl->writeTimeoutTimer = new QTimer(this);
    m_pimpl->writeTimeoutTimer->setSingleShot(true);
    m_pimpl->writeTimeoutTimer->setTimerType(Qt::PreciseTimer);
    connect(m_pimpl->writeTimeoutTimer, &QTimer::timeout, this, &Dispatcher::onWriteTimeout);

    m_pimpl->processReadBufferTimer = new QTimer(this);
    m_pimpl->processReadBufferTimer->setSingleShot(true);
    m_pimpl->processReadBufferTimer->setTimerType(Qt::PreciseTimer);
    connect(m_pimpl->processReadBufferTimer, &QTimer::timeout, this, &Dispatcher::processReadBuffer);

    connect(m_pimpl->port, &QSerialPort::readyRead, this, &Dispatcher::onReadyRead);
    connect(m_pimpl->port, &QSerialPort::bytesWritten, this, &Dispatcher::onBytesWritten);
    connect(m_pimpl->port, &QSerialPort::errorOccurred, this, &Dispatcher::onErrorOccurred);
}

Dispatcher::~Dispatcher()
{
    close();

    QMutexLocker lock(&m_pimpl->accessorsMutex);
    for (auto it = m_pimpl->accessors.cbegin(); it != m_pimpl->accessors.cend(); ++it) {
        delete it.value();
    }
}

Dispatcher::Accessor *Dispatcher::getAccessor(quint8 address)
{
    QMutexLocker lock(&m_pimpl->accessorsMutex);

    if (!m_pimpl->accessors.contains(address)) {
        infoDevice << "Create new accessor for address:" << QString::number(address, 16);
        m_pimpl->accessors.insert(address, new Accessor(*this));
    }

    return m_pimpl->accessors.value(address);
}

Dispatcher::Params Dispatcher::params() const
{
    return m_pimpl->params;
}

QString Dispatcher::lastError() const
{
    return m_pimpl->errorStr;
}

bool Dispatcher::open(const Params &params)
{
    infoDevice << "Opening dispatcher";
    QMutexLocker lock(&m_pimpl->ioMutex);

    if (m_pimpl->port->isOpen()) {
        errDevice << "Serial port is already open";
        setLastError(tr("COM-порт уже открыт"));
        return false;
    }

    m_pimpl->params = params;

    infoDevice << "Serial port:" << m_pimpl->params.portName
               << "Baud rate:" << m_pimpl->params.portBaudRate
               << "Write delay, ms:" << m_pimpl->params.writeDelayMs
               << "Write timeout, ms:" << m_pimpl->params.writeTimeoutMs
               << "Frame interval, ms:" << m_pimpl->params.frameIntervalMs;

    m_pimpl->port->setPortName(m_pimpl->params.portName);
    m_pimpl->port->setBaudRate(m_pimpl->params.portBaudRate);

    if (!m_pimpl->port->open(QIODevice::ReadWrite)) {
        errDevice << m_pimpl->port->errorString();
        setLastError(tr("Не удалось открыть COM-порт"));
        return false;
    }

    infoDevice << "Dispatcher successfully opened";

    emit opened();
    return true;
}

bool Dispatcher::isOpen()
{
    QMutexLocker lock(&m_pimpl->ioMutex);

    return m_pimpl->port->isOpen();
}

void Dispatcher::close()
{
    infoDevice << "Closing dispatcher";
    QMutexLocker lock(&m_pimpl->ioMutex);

    if (!m_pimpl->port->isOpen()) {
        warnDevice << "Dispatcher is not open yet";
        return;
    }

    clear();

    m_pimpl->clearAccessorsReadBuffer();
    m_pimpl->port->close();

    infoDevice << "Dispatcher successfully closed";
    emit closed();
}

bool Dispatcher::reset()
{
    QMutexLocker lock(&m_pimpl->ioMutex);

    m_pimpl->clearAccessorsReadBuffer();
    return setMode(Mode_Echo) && setMode(Mode_Reset) && setMode(Mode_Write);
}

bool Dispatcher::setMode(Dispatcher::Mode mode)
{
    if (this->mode() == mode) {
        warnDevice << "Dispatcher already in" << mode << "mode";
        return true;
    }

    clear();

    switch (mode) {
    case Mode_Write:        
        m_pimpl->setupRtsAndDtr(false, false, 50);
        break;
    case Mode_Read:
        m_pimpl->setupRtsAndDtr(true, true);
        break;
    case Mode_Reset:
        m_pimpl->setupRtsAndDtr(false, true, 500);
        break;
    case Mode_Echo:
        m_pimpl->setupRtsAndDtr(true, false, 500);
        break;
    }

    if (this->mode() != mode) {
        errDevice << "Failed to set mode:" << mode;
        setLastError(tr("Не удалось установить сигналы RTS и DTR"));
        return false;
    }

    return true;
}

Dispatcher::Mode Dispatcher::mode() const
{
    const bool rts = m_pimpl->port->isRequestToSend();
    const bool dtr = m_pimpl->port->isDataTerminalReady();

    if (!rts && !dtr) {
        return Mode_Write;
    } else if (rts && dtr) {
        return Mode_Read;
    } else if (!rts && dtr) {
        return Mode_Reset;
    }

    return Mode_Echo;
}

bool Dispatcher::write(const QByteArray &bytes)
{
    dbgDevice << "Write to serial port:" << bytes.toHex();

    const auto currentMode = mode();
    if (currentMode != Mode_Write) {
        errDevice << "Current mode:" << currentMode << "Expected:" << Mode_Write;
        setLastError(tr("Диспечетр не находится в режиме записи"));
        return false;
    }

    const qint64 bytesWritten = m_pimpl->port->write(bytes);
    if (bytesWritten != bytes.count()) {
        errDevice << "Bytes written:" << bytesWritten << "Expected:" << bytes.count();
        m_pimpl->port->clear(QSerialPort::Output);
        setLastError(tr("Не удалось записать байты в COM-порт"));
        return false;
    }

    dbgDevice << "Bytes successfully placed to serial port buffer";

    m_pimpl->startWriteTimeoutTimer();
    return true;
}

void Dispatcher::clear()
{
    m_pimpl->stopProcessReadBufferTimer();
    m_pimpl->stopWriteTimeoutTimer();

    m_pimpl->readBuffer.clear();
    m_pimpl->port->clear();
}

void Dispatcher::onReadyRead()
{
    dbgDevice << "Bytes available on serial port";

    const auto currentMode = mode();
    if (currentMode != Mode_Read) {
        warnDevice << "Current mode:" << currentMode << "Expected:" << Mode_Read;
        return;
    }

    m_pimpl->stopProcessReadBufferTimer();

    dbgDevice << "Read buffer content before reading from serial port:" << m_pimpl->readBuffer.toHex();

    const QByteArray bytes = m_pimpl->port->readAll();
    m_pimpl->readBuffer.append(bytes);

    dbgDevice << "Read buffer content after reading from serial port:" << m_pimpl->readBuffer.toHex();

    m_pimpl->startProcessReadBufferTimer();
}

void Dispatcher::onBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes)

    dbgDevice << "Bytes" << bytes << "successfully written to serial port";

    const auto currentMode = mode();
    if (currentMode != Mode_Write) {
        warnDevice << "Current mode:" << currentMode << "Expected:" << Mode_Write;
        return;
    }

    auto remainingBytes = m_pimpl->port->bytesToWrite();
    if (!remainingBytes) {
        m_pimpl->stopWriteTimeoutTimer();
        QTimer::singleShot(m_pimpl->params.writePauseMs, this, [this] {
            dbgDevice << "All bytes successfully written";
            emit writeSuccess();
        });
    } else {
        dbgDevice << "Remaining bytes to write:" << remainingBytes;
    }
}

void Dispatcher::onWriteTimeout()
{
    errDevice << "Write timeout occurred";
    m_pimpl->port->clear(QSerialPort::Direction::Output);
    setLastError(QStringLiteral("Write timeout occurred"));
}

void Dispatcher::onErrorOccurred()
{    
    if (m_pimpl->port->error() == QSerialPort::NoError) {
        return;
    }

    QString error = m_pimpl->port->errorString();
    errDevice << "Serial port error occurred:" << error;
    setLastError(error);
}

void Dispatcher::processReadBuffer()
{
    dbgDevice << "Processing dispatcher read buffer";
    QMutexLocker lock(&m_pimpl->accessorsMutex);

    // 2 is minumum length (1 for address and 1 for payload)
    if (m_pimpl->readBuffer.size() < 2) {
        warnDevice << "Dispatcher read buffer size < 2. Cleaning buffer";
        m_pimpl->readBuffer.clear();
        return;
    }

    auto address = static_cast<quint8>(m_pimpl->readBuffer.at(0));

    dbgDevice << "First byte of read buffer:" << QString::number(address, 16);

    if (auto accessor = m_pimpl->accessors.value(address, nullptr)) {
        accessor->setReadBuffer(m_pimpl->readBuffer);
    }

    m_pimpl->readBuffer.clear();
}

void Dispatcher::setLastError(const QString &errorString)
{    
    m_pimpl->errorStr = errorString;
    emit errorOccurred(m_pimpl->errorStr);
}

void Dispatcher::PImpl::startWriteTimeoutTimer()
{
    dbgDevice << "Start write timeout timer";
    writeTimeoutTimer->start(params.writeTimeoutMs);
}

void Dispatcher::PImpl::stopWriteTimeoutTimer()
{
    dbgDevice << "Stop write timeout timer";
    writeTimeoutTimer->stop();
}

void Dispatcher::PImpl::startProcessReadBufferTimer()
{
    dbgDevice << "Start frame interval timer";
    processReadBufferTimer->start(params.frameIntervalMs);
}

void Dispatcher::PImpl::stopProcessReadBufferTimer()
{
    dbgDevice << "Stop frame interval timer";
    processReadBufferTimer->stop();
}

void Dispatcher::PImpl::clearAccessorsReadBuffer()
{
    dbgDevice << "Cleaning accessors read buffer";

    QMutexLocker lock(&accessorsMutex);

    for (auto it = accessors.cbegin(); it != accessors.cend(); ++it) {
        it.value()->setReadBuffer(QByteArray());
    }
}

Dispatcher::Accessor::Accessor(Dispatcher &dispatcher) :
    m_dispatcher(dispatcher)
{

}

bool Dispatcher::Accessor::write(const QByteArray &bytes)
{
    QMutexLocker lock(&m_dispatcher.m_pimpl->ioMutex);

    return doWrite(bytes);
}

QByteArray Dispatcher::Accessor::writeAndRead(const QByteArray &input, quint32 msec)
{
    QMutexLocker lock(&m_dispatcher.m_pimpl->ioMutex);

    if (!doWrite(input)) {
        return QByteArray();
    }

    return doRead(msec);
}

QByteArray Dispatcher::Accessor::read(quint32 msec)
{
    QMutexLocker lock(&m_dispatcher.m_pimpl->ioMutex);

    return doRead(msec);
}

void Dispatcher::Accessor::setReadBuffer(const QByteArray &bytes)
{
    QMutexLocker lock(&m_readMutex);

    m_readBuffer = bytes;
    m_readWaitCondition.wakeAll();
}

bool Dispatcher::Accessor::doWrite(const QByteArray &bytes)
{
    Q_ASSERT(QThread::currentThread() != m_dispatcher.thread());

    dbgDevice << "Switching dispatcher to write mode";
    if (!Toolbox::Invoker::run(&m_dispatcher, &Dispatcher::setMode, Mode_Write).result()) {
        return false;
    }

    if (m_dispatcher.m_pimpl->params.writeDelayMs > 0 && m_dispatcher.m_pimpl->writeDelayTimer.isValid()) {
        auto remainingMs = m_dispatcher.m_pimpl->params.writeDelayMs - m_dispatcher.m_pimpl->writeDelayTimer.elapsed();
        if (remainingMs > 0 && remainingMs <= m_dispatcher.m_pimpl->params.writeDelayMs) {
            dbgDevice << "Sleep before write to dispatcher, ms:" << remainingMs;
            QThread::msleep(static_cast<ulong>(remainingMs));
        }
    }

    DispatcherWriteWatcher watcher(&m_dispatcher);

    dbgDevice << "Write to dispatcher:" << bytes.toHex();

    // Don't wait outcome because if error occurred
    // Dispatcher::errorOccurred signal will be emmitted
    Toolbox::Invoker::run(&m_dispatcher, &Dispatcher::write, bytes);

    bool success = watcher.wait();
    m_dispatcher.m_pimpl->writeDelayTimer.restart();
    return success;
}

QByteArray Dispatcher::Accessor::doRead(quint32 msec)
{
    Q_ASSERT(QThread::currentThread() != m_dispatcher.thread());

    dbgDevice << "Clear accessor read buffer";
    setReadBuffer(QByteArray());

    dbgDevice << "Switching dispatcher to read mode";
    if (!Toolbox::Invoker::run(&m_dispatcher, &Dispatcher::setMode, Mode_Read).result()) {
        return QByteArray();
    }

    QMutexLocker lock(&m_readMutex);

    if (m_readBuffer.isEmpty()) {
        dbgDevice << "Accessor waiting for incoming data for" << msec << "ms";
        m_readWaitCondition.wait(&m_readMutex, msec);
    }

    QByteArray bytes = m_readBuffer;
    m_readBuffer.clear();

    dbgDevice << "Accessor read buffer content:" << bytes.toHex();
    return bytes;
}

Dispatcher::Params::Params() :
    portName(QStringLiteral("COM1")),
    portBaudRate(9600),
    writeTimeoutMs(1000),
    writeDelayMs(150),
    writePauseMs(50),
    frameIntervalMs(10)
{

}

bool Dispatcher::PImpl::setupRtsAndDtr(bool rts, bool dtr, quint32 msecAfter)
{
    bool result = port->setRequestToSend(rts);
    result = port->setDataTerminalReady(dtr) && result;

    if (result && msecAfter) {
        QThread::msleep(msecAfter);
    }

    return result;
}

DispatcherWriteWatcher::DispatcherWriteWatcher(Dispatcher *dispatcher, QObject *parent) :
    QObject(parent),
    m_result(ResultUnknown)
{
    connect(dispatcher, &Dispatcher::writeSuccess,
            this, &DispatcherWriteWatcher::onWriteSuccess, Qt::DirectConnection);
    connect(dispatcher, &Dispatcher::errorOccurred,
            this, &DispatcherWriteWatcher::onErrorOccurred, Qt::DirectConnection);
}

bool DispatcherWriteWatcher::wait()
{
    QMutexLocker lock(&m_mutex);

    if (m_result != ResultUnknown) {
        return m_result;
    }

    m_waitCondition.wait(&m_mutex);
    return m_result == ResultSuccess;
}

void DispatcherWriteWatcher::onWriteSuccess()
{
    QMutexLocker lock(&m_mutex);

    if (m_result != ResultUnknown) {
        return;
    }

    m_result = ResultSuccess;
    m_waitCondition.wakeAll();
}

void DispatcherWriteWatcher::onErrorOccurred()
{
    QMutexLocker lock(&m_mutex);

    if (m_result != ResultUnknown) {
        return;
    }

    m_result = ResultError;
    m_waitCondition.wakeAll();
}

QDebug operator<<(QDebug debug, Dispatcher::Mode mode)
{
    switch (mode) {
    case Dispatcher::Mode_Write:
        debug << "Write";
        break;
    case Dispatcher::Mode_Read:
        debug << "Read";
        break;
    case Dispatcher::Mode_Reset:
        debug << "Reset";
        break;
    case Dispatcher::Mode_Echo:
        debug << "Echo";
        break;
    default:
        debug << "Unknown";
    }

    return debug;
}

#include "Dispatcher.moc"
