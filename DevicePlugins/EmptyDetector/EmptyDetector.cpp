#include "EmptyDetector.h"

#include <QThread>
#include <random>

#include <Device/DeviceLogging.h>

const qreal DEFAULT_PIXEL_SIZE = 0.2;
const int DEFAULT_WIDTH = 576;
const qreal DEFAULT_CHARGE_TIME = 3;

const QString DETECTOR_WIDTH = QStringLiteral("main/width");
const QString DETECTOR_CHARGE_TIME = QStringLiteral("main/charge_time");
const QString DETECTOR_PIXEL_SIZE = QStringLiteral("main/pixel_size");

EmptyDetector::EmptyDetector(QObject *parent) : Detector(parent)
{

}

EmptyDetector::~EmptyDetector()
{

}

bool EmptyDetector::doTestConnection()
{
    return true;
}

Detector::Frame EmptyDetector::makeFrame(quint32 lines, float value)
{
    auto props = properties();

    Frame result(props.width * lines);
    std::default_random_engine generator;
    std::poisson_distribution<ushort> distribution(value);
    for (int i = 0; i < result.size(); i++) {
        result[i] = distribution(generator);
    }
    QThread::msleep(static_cast<unsigned int>(props.chargeTimeMsec * lines));
    return result;

}

bool EmptyDetector::doOpen()
{
    Properties props;

    auto &cfg = currentConfiguration();

    props.width = cfg.value(DETECTOR_WIDTH).toInt();
    props.chargeTimeMsec = cfg.value(DETECTOR_CHARGE_TIME).toReal();
    props.pixelSizeMm.setWidth(cfg.value(DETECTOR_PIXEL_SIZE).toReal());
    props.pixelSizeMm.setHeight(cfg.value(DETECTOR_PIXEL_SIZE).toReal());

    if (props.width < 1) {
        warnDevice << "Width out of range";
        props.width = DEFAULT_WIDTH;
    }

    if (props.chargeTimeMsec <= 0) {
        warnDevice << "Charge time out of range";
        props.chargeTimeMsec = DEFAULT_CHARGE_TIME;
    }

    if (props.pixelSizeMm.width() <= 0) {
        warnDevice << "Pixel width out of range";
        props.pixelSizeMm.setWidth(DEFAULT_PIXEL_SIZE);
    }

    props.pixelSizeMm.setHeight(props.pixelSizeMm.width());

    setProperties(props);

    return true;
}

void EmptyDetector::doClose()
{

}

bool EmptyDetector::doPrepare()
{
    return true;
}

bool EmptyDetector::doCapture()
{
    setLastCapturedFrame(makeFrame(currentLines(), 65535));
    return true;
}

DeviceConfiguration EmptyDetector::defaultConfiguration() const
{
    DeviceConfiguration conf;
    conf.insert(DETECTOR_WIDTH, DEFAULT_WIDTH, tr("Ширина, пиксели [>0]"));
    conf.insert(DETECTOR_CHARGE_TIME, DEFAULT_CHARGE_TIME, tr("Время накопления, мс [>0.0]"));
    conf.insert(DETECTOR_PIXEL_SIZE, DEFAULT_PIXEL_SIZE, tr("Размер пикселя, мм [>0.0]"));
    return conf;
}
