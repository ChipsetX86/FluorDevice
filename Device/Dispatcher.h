#ifndef DISPATCHER_H
#define DISPATCHER_H

#include "DeviceGlobal.h"

#include <QObject>
#include <QByteArray>
#include <QMutex>
#include <QWaitCondition>
#include <QScopedPointer>

class DEVICELIB_EXPORT Dispatcher final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Dispatcher)
public:
    struct DEVICELIB_EXPORT Params
    {
        Params();
        QString portName;
        quint32 portBaudRate;
        quint16 writeTimeoutMs;
        quint16 writeDelayMs;
        quint8  writePauseMs;
        quint8  frameIntervalMs;
    };

    class DEVICELIB_EXPORT Accessor
    {
        Q_DISABLE_COPY(Accessor)
    public:
        Accessor(Dispatcher &dispatcher);

        bool write(const QByteArray &bytes);
        QByteArray read(quint32 msec = 1000);
        QByteArray writeAndRead(const QByteArray &input, quint32 msec = 1000);
    private:
        friend class Dispatcher;

        void setReadBuffer(const QByteArray &bytes);
        bool doWrite(const QByteArray &bytes);
        QByteArray doRead(quint32 msec);

        Dispatcher &m_dispatcher;
        QMutex m_readMutex;
        QWaitCondition m_readWaitCondition;
        QByteArray m_readBuffer;
    };

    explicit Dispatcher(QObject *parent = nullptr);
    ~Dispatcher();

    Accessor *getAccessor(quint8 address);

    Params params() const;

    QString lastError() const;
public slots:
    bool open(const Dispatcher::Params &params);
    bool isOpen();
    void close();
    bool reset();
signals:
    void opened();
    void closed();
    void writeSuccess();
    void errorOccurred(QString error);
private slots:
    void onReadyRead();
    void onBytesWritten(qint64 bytes);
    void onWriteTimeout();
    void onErrorOccurred();
    void processReadBuffer();
private:
    enum Mode {
        Mode_Write,
        Mode_Read,
        Mode_Reset,
        Mode_Echo
    };

    bool setMode(Mode mode);
    Mode mode() const;
    bool write(const QByteArray &bytes);
    void clear();

    void setLastError(const QString &errorString);

    friend QDebug operator<<(QDebug, Dispatcher::Mode);

    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

QDebug operator<<(QDebug, Dispatcher::Mode);

#endif // DISPATCHER_H
