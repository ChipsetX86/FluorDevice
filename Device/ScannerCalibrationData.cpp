#include "ScannerCalibrationData.h"

#include <NpApplication/Application.h>
#include <Settings/LocalSettings.h>

#include "ScanningModesCollection.h"
#include "FlatFieldCorrection.h"

using namespace Nauchpribor;

const float kRecommendedRatio = 0.9f;

ScannerCalibrationData::~ScannerCalibrationData()
{
    for (auto it = m_ffc.cbegin(); it != m_ffc.cend(); ++it) {
        delete it.value();
    }
}

ScannerCalibrationData &ScannerCalibrationData::instance()
{
    static ScannerCalibrationData i;
    return i;
}

bool ScannerCalibrationData::updateIsRecommended() const
{
    auto &s = LocalSettings::instance();
    return isExpired(qRound(s.scannerCalibrationLifeTime() * kRecommendedRatio));
}

bool ScannerCalibrationData::updateIsRequired() const
{
    auto &s = LocalSettings::instance();
    return isExpired(s.scannerCalibrationLifeTime());
}

bool ScannerCalibrationData::isExpired(int sec) const
{
    if (m_ffc.isEmpty()) {
        return false;
    }
    
    QDateTime lastCalibration;
    
    for (const auto &ffc : qAsConst(m_ffc)) {
        QDateTime calibration = ffc->lastCalibrationDateTime();
        if (!calibration.isValid()) {
            lastCalibration = QDateTime();
            break;
        }

        if (lastCalibration.isNull() || calibration < lastCalibration) {
            lastCalibration = calibration;
        }
    }

    return !lastCalibration.isValid() ||
            (sec > 0 && lastCalibration.addSecs(sec) < QDateTime::currentDateTime());
}

bool ScannerCalibrationData::update(const QString &scanningModeUuid, const Image &image, const Image &dark, int width)
{
    int height = image.size() / width;
    const int pos = width * qRound(height * 0.25);
    const int len = width * qRound(height * 0.5);

    FlatFieldCorrection *ffc = m_ffc.value(scanningModeUuid);
    return ffc && ffc->calibrate(image.mid(pos, len), dark, width);
}

bool ScannerCalibrationData::apply(const QString &scanningModeUuid, Image &image, const Image &dark, int width)
{
    if (image.isEmpty() || image.size() % width) {
        return false;
    }

    FlatFieldCorrection *ffc = m_ffc.value(scanningModeUuid);
    return ffc && ffc->correct(image, dark, width);
}

ScannerCalibrationData::ScannerCalibrationData()
{
    const ScanningModesCollection::ItemsList modes = ScanningModesCollection::instance().list();
    for (const auto &mode : modes) {
        const QString uuid = mode.uuid();
        QString filename = npApp->permanentDataFilename(QStringLiteral("%1.gains").arg(uuid));
        m_ffc.insert(uuid, new FlatFieldCorrection(filename));
    }
}
