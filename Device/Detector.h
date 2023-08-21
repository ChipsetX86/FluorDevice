#ifndef DETECTOR_H
#define DETECTOR_H

#include "Device.h"

#include <QVector>
#include <QSizeF>

class DEVICELIB_EXPORT Detector: public Device
{
    Q_OBJECT
    Q_DISABLE_COPY(Detector)
public:
    struct DEVICELIB_EXPORT Properties
    {
        Properties();
        int width;
        QSizeF pixelSizeMm;
        qreal chargeTimeMsec;
    };

    typedef QVector<float> Frame;

    explicit Detector(QObject *parent = nullptr);
    ~Detector() override;
    Properties properties() const;
    Frame lastCapturedFrame() const;
public slots:
    bool prepare(quint32 lines);
    bool capture();
signals:
    void prepared(QPrivateSignal);
    void captured(Detector::Frame frame, QPrivateSignal);
protected:
    quint32 currentLines() const;
    void setProperties(const Properties &properties);
    void setLastCapturedFrame(const Frame &frame);

    virtual bool doPrepare() = 0;
    virtual bool doCapture() = 0;
private:
    void onClosed() override final;

    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif
