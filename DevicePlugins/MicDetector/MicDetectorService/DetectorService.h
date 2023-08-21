#ifndef DETECTORSERVICE_H
#define DETECTORSERVICE_H

#include <QObject>
#include <QVariantMap>
#include "MicDetectorWrapper.h"

class DetectorService : public QObject
{
    Q_OBJECT
public:
    explicit DetectorService(MicDetectorWrapper *detectorWrapper, QObject *parent = nullptr);
    Q_INVOKABLE QVariantMap loadMicLibrary();
    Q_INVOKABLE QVariantMap hardwareInit();
    Q_INVOKABLE QVariantMap prepareReading(int linesCount, qreal chargeTime);
    Q_INVOKABLE QVariantMap readHardwareInfo();
    Q_INVOKABLE QVariantMap readData();
    Q_INVOKABLE QVariantMap hardwareShutdown();
private:
    MicDetectorWrapper *m_detector;
    QVariantMap prepareAnswer(bool isSuccess) const;
};

#endif // DETECTORSERVICE_H
