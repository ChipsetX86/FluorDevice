#include <QDebug>
#include <QDataStream>
#include "DetectorService.h"
#include "../Common.h"

DetectorService::DetectorService(MicDetectorWrapper *detectorWrapper, QObject *parent)
    : QObject(parent),
      m_detector(detectorWrapper)
{

}

QVariantMap DetectorService::loadMicLibrary()
{
    return prepareAnswer(m_detector->loadMicLibrary());
}

QVariantMap DetectorService::hardwareInit()
{
    return prepareAnswer(m_detector->hardwareInit());
}

QVariantMap DetectorService::prepareReading(int linesCount, qreal chargeTime)
{
    return prepareAnswer(m_detector->prepareReading(linesCount, chargeTime));
}

QVariantMap DetectorService::readHardwareInfo()
{
    MicDetectorWrapper::HardwareInfo info;
    bool isSuccess = m_detector->readHardwareInfo(info);
    QVariantMap map = prepareAnswer(isSuccess);
    if (isSuccess) {
        map[NAME_FIELD_WIDTH_DETECTOR] = info.widthPx;
    }
    return map;
}

QVariantMap DetectorService::readData()
{
    MicFrame frame;
    bool isSuccess = m_detector->readData(frame);
    QVariantMap map = prepareAnswer(isSuccess);
    if (isSuccess) {
        QByteArray bytesScan;
        QDataStream stream(&bytesScan, QIODevice::WriteOnly);
        stream << frame;
        map[NAME_FIELD_FRAME] = QString(bytesScan.toBase64());
    }
    return map;
}

QVariantMap DetectorService::hardwareShutdown()
{
    return prepareAnswer(m_detector->hardwareShutdown());
}

QVariantMap DetectorService::prepareAnswer(bool isSuccess) const
{
    QVariantMap map;
    map[NAME_FIELD_IS_SUCCESS] = isSuccess;
    if (!isSuccess) {
        map[NAME_FIELD_TEXT_ERROR] = m_detector->lastError();
    }
    return map;
}
