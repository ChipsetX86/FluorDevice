#ifndef DEVICE_SCANNERCALIBRATIONDATA_H
#define DEVICE_SCANNERCALIBRATIONDATA_H

#include "DeviceGlobal.h"

#include <QMap>
#include <QVector>

class FlatFieldCorrection;

class DEVICELIB_EXPORT ScannerCalibrationData final
{
public:
    static ScannerCalibrationData &instance();

    typedef QVector<float> Image;

    ~ScannerCalibrationData();

    bool updateIsRecommended() const;
    bool updateIsRequired() const;

    bool update(const QString &scanningModeUuid, const Image &image,
                const Image &dark, int width);

    bool apply(const QString &scanningModeUuid, Image &image,
               const Image &dark, int width);
private:
    ScannerCalibrationData();
    bool isExpired(int sec) const;

    QMap<QString, FlatFieldCorrection *> m_ffc;
};

#endif // DEVICE_SCANNERCALIBRATIONDATA_H
