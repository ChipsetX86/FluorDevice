#ifndef FLATFIELDCORRECTION_H
#define FLATFIELDCORRECTION_H

#include "DeviceGlobal.h"

#include <QVector>
#include <QDateTime>
#include <QMutex>

class DEVICELIB_EXPORT FlatFieldCorrection
{    
    Q_DISABLE_COPY(FlatFieldCorrection)
public:
    explicit FlatFieldCorrection(const QString &filename);
    QDateTime lastCalibrationDateTime() const;
    bool calibrate(const QVector<float> &img,
                   const QVector<float> &darkFrame,
                   int width);
    bool correct(QVector<float> &img,
                 const QVector<float> &darkFrame,
                 int width);
    int width() const;
private:
    void setLastCalibrationDateTime(const QDateTime &lastCalibration);

    void loadGains();
    bool saveGains(const QVector<float> &gains);
    QVector<float> ratesFromDarkFrame(const QVector<float>& darkFrame, int width) const;

    QString m_filename;
    mutable QMutex m_lastCalibrationMutex;
    QDateTime m_lastCalibration;
    QVector<float> m_gains;
};

#endif // FLATFIELDCORRECTION_H
