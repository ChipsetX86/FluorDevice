#include "SibelGenericDetectorPrivate.h"

#include <QScopedArrayPointer>
#include <QScopeGuard>
#include <QThread>

#include <Device/DeviceLogging.h>

const int minLatency    = 2;
const int maxLatency    = 255;
const int usbBulkSize   = 512;
const int minBufferSize = usbBulkSize;
const int maxBufferSize = 64 * 1024;

const int SibelGenericDetectorPrivate::defaultLatency    = 2;
const int SibelGenericDetectorPrivate::defaultBufferSize = 16 * 1024;

const uint maxEmptyFtRead = 10;
const QMap<uint, uint> odParamMap = {
    {2, 1}, {3, 6}, {4, 3},
    {5, 4}, {6, 7}, {7, 5},
    {8, 2}, {10, 0}
};

const QStringList supportPartsInSerialNumber {"PR", "SIBEL"};

namespace {
    struct FtError final
    {
    public:
        FtError(FT_STATUS s) : m_s(s)
        {

        }

        FT_STATUS code() const
        {
            return m_s;
        }

        operator bool() const
        {
            return m_s != FT_OK;
        }
    private:
        FT_STATUS m_s;
    };
}

SibelGenericDetectorPrivate::SibelGenericDetectorPrivate(int width, int batches, int depth,
                                                         int bitsForLinesCount, const QSizeF &pixelSize,
                                                         QObject *parent) :
    QObject(parent),    
    m_width(width),
    m_batches(batches),
    m_depth(depth),
    m_bitsForLinesCount(bitsForLinesCount),
    m_pixelSize(pixelSize),
    m_handle(nullptr),
    m_latency(defaultLatency),
    m_bufferSize(defaultBufferSize),
    m_frequency(0),
    m_linesCount(0),
    m_snapshotSize(0)
{

}

const SibelGenericDetectorPrivate::Configuration &SibelGenericDetectorPrivate::configuration() const
{
    return m_conf;
}

qreal SibelGenericDetectorPrivate::frequency() const
{
    return m_frequency;
}

bool SibelGenericDetectorPrivate::writeBytes(uchar *buffer, uint size)
{
    DWORD bytesWritten;
    FtError e = FT_Write(m_handle, buffer, size, &bytesWritten);
    if (e || bytesWritten != size) {
        setLastError(tr("Ошибка метода FT_Write (%1, %2)").arg(size).arg(bytesWritten));
        return false;
    }

    return true;
}

bool SibelGenericDetectorPrivate::setLinesCount(int lines)
{
    if (m_bitsForLinesCount % 4 || m_bitsForLinesCount < 4 || m_bitsForLinesCount > 32) {
        setLastError(tr("Неподдерживаемое кол-во битов для количества строк"));
        return false;
    }

    quint32 maxValue = 0;
    for (int i = 0; i < m_bitsForLinesCount; ++i) {
        maxValue <<= 1;
        maxValue |= 1;
    }

    if (lines < 1 || static_cast<quint32>(lines) > maxValue) {
        setLastError(tr("Задано неверное количество строк"));
        return false;
    }

    const int commandLength = m_bitsForLinesCount / 4;
    QScopedArrayPointer<uchar> command(new uchar[commandLength]);
    for (int i = 0; i < commandLength; ++i) {
        command[i] = (((static_cast<quint32>(lines) >> (i * 4)) << 4) & 0xF0) | 0x0E;
    }

    return writeBytes(command.data(), commandLength);
}

bool SibelGenericDetectorPrivate::isOpen() const
{
    return m_handle != nullptr;
}

SibelGenericDetectorPrivate::~SibelGenericDetectorPrivate()
{

}

bool SibelGenericDetectorPrivate::open()
{
    if (isOpen()) {
        setLastError(tr("Детектор уже открыт"));
        return false;
    }

    QString serialNumber;
    if (!serialNumberDetector(serialNumber)) {
        return false;
    }

    if (auto e = FtError(FT_OpenEx((PVOID)serialNumber.toStdString().data(),
                                   FT_OPEN_BY_SERIAL_NUMBER,
                                   &m_handle))) {
        setLastError(tr("Ошибка метода FT_OpenEx (%1)").arg(e.code()));
        return false;
    }

    auto closeGuard = qScopeGuard([this] {
        close();
    });

    if (auto e = FtError(FT_ResetDevice(m_handle))) {
        setLastError(tr("Ошибка метода FT_ResetDevice (%1)").arg(e.code()));
        return false;
    }

    if (auto e = FtError(FT_Purge(m_handle, FT_PURGE_RX | FT_PURGE_TX))) {
        setLastError(tr("Ошибка метода FT_Purge (%1)").arg(e.code()));
        return false;
    }

    if (auto e = FtError(FT_SetLatencyTimer(m_handle, m_latency))) {
        setLastError(tr("Ошибка метода FT_SetLatencyTimer (%1)").arg(e.code()));
        return false;
    }

    if (auto e = FtError(FT_SetBitMode(m_handle, 0xFF, FT_BITMODE_RESET))) {
        setLastError(tr("Ошибка метода FT_SetBitMode (%1)").arg(e.code()));
        return false;
    }

    if (auto e = FtError(FT_SetBitMode(m_handle, 0xFF, FT_BITMODE_SYNC_FIFO))) {
        setLastError(tr("Ошибка метода FT_SetBitMode (%1)").arg(e.code()));
        return false;
    }

    closeGuard.dismiss();
    return true;
}

void SibelGenericDetectorPrivate::close()
{
    if (!isOpen()) {
        return;
    }

    FT_Close(m_handle);
    m_handle = nullptr;
}

bool SibelGenericDetectorPrivate::testConnection()
{
    if (!isOpen()) {
        setLastError(tr("Детектор не открыт"));
        return false;
    }

    ULONG modemStatus;

    if (auto e = FtError(FT_GetModemStatus(m_handle, &modemStatus))) {
        setLastError(tr("Ошибка метода FT_GetModemStatus (%1)").arg(e.code()));
        return false;
    }

    return true;
}

bool SibelGenericDetectorPrivate::prepare(int linesCount)
{
    if (!isOpen()) {
        setLastError(tr("Детектор не открыт"));
        return false;
    }

    if (!setLinesCount(linesCount)) {
        return false;
    }

    m_linesCount = linesCount;
    m_snapshotSize = static_cast<uint>(m_linesCount * width() * depth());
    m_snapshot.reset(new uchar[m_snapshotSize]);

    return true;
}

bool SibelGenericDetectorPrivate::setConfiguration(const SibelGenericDetectorPrivate::Configuration &conf)
{
    if (conf.vdw < 4 || conf.vdw > 511) {
        setLastError(tr("Неверное значение VDW (%1)").arg(conf.vdw));
        return false;
    }

    if (conf.rdw < 1 || conf.rdw > 127) {
        setLastError(tr("Неверное значение RDW (%1)").arg(conf.rdw));
        return false;
    }

    if (!odParamMap.contains(conf.od)) {
        setLastError(tr("Неверное значение OD (%1)").arg(conf.od));
        return false;
    }

    if (conf.ttl > 1) {
        setLastError(tr("Неверное значение TTL (%1)").arg(conf.ttl));
        return false;
    }

    if (conf.c0 > 1) {
        setLastError(tr("Неверное значение C0 (%1)").arg(conf.c0));
        return false;
    }

    if (conf.c1 > 1) {
        setLastError(tr("Неверное значение C1 (%1)").arg(conf.c1));
        return false;
    }

    quint32 confReg = 0;
    const quint32 mappedOd = odParamMap.value(conf.od);

    confReg |= conf.rdw;
    confReg |= (conf.vdw << 7);
    confReg |= (mappedOd << 16);
    confReg |= (conf.ttl << 21);
    confReg |= (conf.c0 << 22);
    confReg |= (conf.c1 << 23);

    if (conf.gain) {
        confReg |= (static_cast<quint32>(1) << 24);
    }

    if (!conf.binning) {
        confReg |= (static_cast<quint32>(1) << 25);
    }

    const int commandLength = 8;
    uchar command[commandLength];
    for (int i = 0; i < commandLength; ++i) {
        command[i] = (((confReg >> (i * 4)) << 4) & 0xF0) | 0x0D;
    }

    if (!writeBytes(command, sizeof(command))) {
        return false;
    }

    m_frequency = 15.f * 2 * (conf.vdw + 8) / ((conf.rdw + 2) * conf.od);
    m_conf = conf;

    infoDevice << "Sibel detector frequency is" << m_frequency << "Mhz";

    QThread::usleep(100); // by ICS-307 datasheet (10 ms)
    return true;
}

bool SibelGenericDetectorPrivate::startFrame()
{
    uchar command = 0x0F;
    int r = writeBytes(&command, sizeof(command));
    return r;
}

bool SibelGenericDetectorPrivate::serialNumberDetector(QString &serialOut)
{
    DWORD countDevice;
    if (auto e = FtError(FT_CreateDeviceInfoList(&countDevice))) {
        setLastError(tr("Ошибка метода FT_CreateDeviceInfoList (%1)").arg(e.code()));
        return false;
    }

    if (!countDevice) {
        setLastError(tr("FTDI устройство не найдено"));
        return false;
    }

    infoDevice << QStringLiteral("Found %1 FTDI devices").arg(countDevice);

    QScopedArrayPointer<FT_DEVICE_LIST_INFO_NODE> infoDevices(new FT_DEVICE_LIST_INFO_NODE[countDevice]);

    if (auto e = FtError(FT_GetDeviceInfoList(infoDevices.data(), &countDevice))) {
        setLastError(tr("Ошибка метода FT_GetDeviceInfoList (%1)").arg(e.code()));
        return false;
    }

    for (uint i = 0; i < countDevice; ++i) {
        QString cSerial(infoDevices[i].SerialNumber);
        for (int k = 0; k < supportPartsInSerialNumber.size(); ++k)
            if (cSerial.indexOf(supportPartsInSerialNumber.at(k), 0, Qt::CaseInsensitive) >= 0) {
                serialOut = cSerial;
                infoDevice << "Found FTDI device. Serial number: " << cSerial;
                return true;
            }
      }

    return false;
}

bool SibelGenericDetectorPrivate::takeSnapshot()
{
    if (!startFrame()) {
        setLastError(tr("Не удалось начать снимок"));
        return false;
    }

    uint emptyReadCounter = 0;
    uint totalReceivedBytes = 0;

    while (totalReceivedBytes < m_snapshotSize) {
        DWORD receivedBytes = 0;
        FtError e = FT_Read(m_handle, m_snapshot.data() + totalReceivedBytes,
                            m_snapshotSize - totalReceivedBytes, &receivedBytes);
        if (e) {
            setLastError(tr("Ошибка метода FT_Read (%1)").arg(e.code()));
            return false;
        }

        totalReceivedBytes += receivedBytes;

        if (!receivedBytes) {            
            if (++emptyReadCounter >= maxEmptyFtRead) {
                break;
            }
        } else {
            emptyReadCounter = 0;
        }
    }

    if (m_snapshotSize != totalReceivedBytes) {
        setLastError(tr("Полученный размер буфера (%1) "
                        "не равен ожидаемому (%2)").arg(totalReceivedBytes).arg(m_snapshotSize));
        return false;
    }

    return true;
}

QVector<float> SibelGenericDetectorPrivate::decodeSnapshot() const
{
    const int width = this->width();

    QVector<float> output;
    output.reserve(width * m_linesCount);

    auto pixels = reinterpret_cast<const ushort *>(m_snapshot.data());
    for (int i = m_linesCount - 1; i >= 0; --i) {
        for (int j = 0; j < m_batches; ++j) {
            for (int k = 0, matrixSize = width / m_batches; k < matrixSize; ++k) {
                output.append(pixels[width * i + k * m_batches + j]);
            }
        }
    }

    return output;
}

bool SibelGenericDetectorPrivate::setLatency(int value)
{
    if (value < minLatency || value > maxLatency) {
        setLastError(tr("Неверное значение задержки детектора"));
        return false;
    }

    m_latency = value;
    return true;
}

bool SibelGenericDetectorPrivate::setBufferSize(int value)
{
    if (value < minBufferSize || value > maxBufferSize || value % usbBulkSize) {
        setLastError(tr("Неверное значение буфера детектора"));
        return false;
    }

    m_bufferSize = value;
    return true;
}

SibelDetectorPR2048::SibelDetectorPR2048(QObject *parent) :
    SibelGenericDetectorPrivate(2304, 3, 2, 16, QSizeF(0.18, 0.18), parent)
{

}

qreal SibelDetectorPR2048::chargeTime() const
{
    const qreal f = frequency();
    return f > 0 ? 40 / f : 0;
}

SibelDetectorPR4096::SibelDetectorPR4096(QObject *parent) :
    SibelGenericDetectorPrivate(4096, 4, 2, 16, QSizeF(0.1, 0.1), parent)
{

}

int SibelDetectorPR4096::width() const
{
    return SibelGenericDetectorPrivate::width() / (isBinningEnabled() ? 2 : 1);
}

QSizeF SibelDetectorPR4096::pixelSize() const
{
    return SibelGenericDetectorPrivate::pixelSize() * (isBinningEnabled() ? 2 : 1);
}

qreal SibelDetectorPR4096::chargeTime() const
{
    const qreal f = frequency();
    return f > 0 ? 48 / f / (isBinningEnabled() ? 2 : 1) * 1.4f : 0;
}

bool SibelDetectorPR4096::isBinningEnabled() const
{
    return configuration().binning;
}

SibelDetectorPR4608::SibelDetectorPR4608(QObject *parent) :
    SibelGenericDetectorPrivate(4608, 3, 2, 16, QSizeF(0.1, 0.1), parent)
{

}

qreal SibelDetectorPR4608::chargeTime() const
{
    const qreal f = frequency();
    return f > 0 ? 40 / f * 1.995f : 0;
}
