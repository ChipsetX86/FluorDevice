#include "SibelGenericDetector.h"

#include "SibelGenericDetectorPrivate.h"

namespace {
    const QString RDW = QStringLiteral("configuration/rdw");
    const QString VDW = QStringLiteral("configuration/vdw");
    const QString OD = QStringLiteral("configuration/od");
    const QString TTL = QStringLiteral("configuration/ttl");
    const QString C0 = QStringLiteral("configuration/c0");
    const QString C1 = QStringLiteral("configuration/c1");
    const QString GAIN = QStringLiteral("configuration/gain");
    const QString BINNING = QStringLiteral("configuration/binning");
    const QString MODEL = QStringLiteral("main/model");
    const QString LATENCY = QStringLiteral("main/latency");
    const QString BUFFER_SIZE = QStringLiteral("main/buffer_size");
    const QString CORRECTION = QStringLiteral("main/correction");

    SibelGenericDetectorPrivate::Configuration readConfiguration(DeviceConfiguration &cfg)
    {
        SibelGenericDetectorPrivate::Configuration result;

        result.rdw = cfg.value(RDW).toUInt();
        result.vdw = cfg.value(VDW).toUInt();
        result.od = cfg.value(OD).toUInt();
        result.ttl = cfg.value(TTL).toUInt();
        result.c0 = cfg.value(C0).toUInt();
        result.c1 = cfg.value(C1).toUInt();
        result.gain = cfg.value(GAIN).toBool();
        result.binning = cfg.value(BINNING).toBool();

        return result;
    }
}

SibelGenericDetector::SibelGenericDetector(QObject *parent) :
    Detector(parent)
{

}

SibelGenericDetector::~SibelGenericDetector()
{

}

bool SibelGenericDetector::doOpen()
{
    auto &cfg = currentConfiguration();

    QScopedPointer<SibelGenericDetectorPrivate> tmp;

    switch (cfg.value(MODEL).toInt()) {
    case 2048:
        tmp.reset(new SibelDetectorPR2048);
        break;
    case 4096:
        tmp.reset(new SibelDetectorPR4096);
        break;
    case 4608:
        tmp.reset(new SibelDetectorPR4608);
        break;
    }

    if (!tmp) {
        setLastError(tr("Неизвестная модель детектора"));
        return false;
    }

    if (!tmp->setLatency(cfg.value(LATENCY).toInt())) {
        setLastError(tmp->lastError());
        return false;
    }

    if (!tmp->setBufferSize(cfg.value(BUFFER_SIZE).toInt())) {
        setLastError(tmp->lastError());
        return false;
    }

    m_pimpl.reset(tmp.take());

    if (!m_pimpl->open()) {
        setLastError(m_pimpl->lastError());
        return false;
    }

    if (!m_pimpl->setConfiguration(readConfiguration(cfg))) {
        setLastError(m_pimpl->lastError());
        return false;
    }

    Properties props;

    props.width = m_pimpl->width();
    props.pixelSizeMm = m_pimpl->pixelSize();
    props.chargeTimeMsec = m_pimpl->chargeTime();

    setProperties(props);

    return true;
}

void SibelGenericDetector::doClose()
{
    m_pimpl->close();
    m_pimpl.reset();
}

bool SibelGenericDetector::doTestConnection()
{
    if (!m_pimpl->testConnection()) {
        setLastError(m_pimpl->lastError());
        return false;
    }

    return true;
}

bool SibelGenericDetector::doPrepare()
{
    if (!m_pimpl->prepare(currentLines())) {
        setLastError(m_pimpl->lastError());
        return false;
    }

    return true;
}

bool SibelGenericDetector::doCapture()
{
    if (!m_pimpl->takeSnapshot()) {
        setLastError(m_pimpl->lastError());
        return false;
    }

    QVector<float> data = m_pimpl->decodeSnapshot();

    if (int correction = currentConfiguration().value(CORRECTION).toInt()) {
        const int batches = m_pimpl->batches();
        const int batchSize = m_pimpl->width() / batches;
        const int width = batches * batchSize;
        const int lines = data.size() / width;

        if (lines > batches - 1) {
            if (correction > 0) {
                for (int i = 1; i < lines; ++i) {
                    for (int j = 1; j < batches; ++j) {
                        for (int k = 0; k < batchSize; ++k) {
                            const int srcIdx = i * width + j * batchSize + k;
                            const int dstIdx = srcIdx - width * j;

                            if (dstIdx < 0) {
                                break;
                            }

                            data[dstIdx] = data.at(srcIdx);
                        }
                    }
                }

                data.resize(data.size() - width * (batches - 1));
            } else {
                for (int i = lines - 2; i > 0; --i) {
                    for (int j = 1; j < batches; ++j) {
                        for (int k = 0; k < batchSize; ++k) {
                            const int srcIdx = i * width + j * batchSize + k;
                            const int dstIdx = srcIdx + width * j;

                            if (dstIdx >= data.size()) {
                                break;
                            }

                            data[dstIdx] = data.at(srcIdx);
                        }
                    }
                }

                data = data.mid(width * (batches - 1));
            }
        }
    }

    setLastCapturedFrame(Frame(data));
    return true;
}

DeviceConfiguration SibelGenericDetector::defaultConfiguration() const
{
    DeviceConfiguration conf;
    conf.insert(RDW, 14, QStringLiteral("RDW [1-127]"));
    conf.insert(VDW, 24, QStringLiteral("VDW [4-511]"));
    conf.insert(OD, 4, QStringLiteral("OD [2-8,10]"));
    conf.insert(TTL, 1, QStringLiteral("TTL [0-1]"));
    conf.insert(C0, 0, QStringLiteral("C0 [0-1]"));
    conf.insert(C1, 0, QStringLiteral("C1 [0-1]"));
    conf.insert(GAIN, false, tr("Усиление"));
    conf.insert(BINNING, false, tr("Биннинг"));
    conf.insert(MODEL, 4096, tr("Модель детектора [2048/4096/4608]"));
    conf.insert(LATENCY, SibelGenericDetectorPrivate::defaultLatency,
                         tr("Задержка, мс [2-255]"));
    conf.insert(BUFFER_SIZE, SibelGenericDetectorPrivate::defaultBufferSize,
                             tr("Размер буфера, байт [512,1024,...,65536]"));
    conf.insert(CORRECTION, 0, tr("Коррекция геометрии снимка [-1/0/1]"));
    return conf;
}
