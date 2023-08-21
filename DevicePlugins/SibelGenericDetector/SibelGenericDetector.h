#ifndef SIBELGENERICDETECTOR_H
#define SIBELGENERICDETECTOR_H

#include <Device/Detector.h>

#include <QVariant>

class SibelGenericDetectorPrivate;

class SibelGenericDetector : public Detector
{
    Q_OBJECT
    Q_DISABLE_COPY(SibelGenericDetector)
public:
    explicit SibelGenericDetector(QObject *parent = nullptr);
    ~SibelGenericDetector() override;
    DeviceConfiguration defaultConfiguration() const override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doTestConnection() override;
    bool doPrepare() override;
    bool doCapture() override;
private:
    QVariant configuration(const QString &name, const QVariant &defaultValue = QVariant()) const;
    QScopedPointer<SibelGenericDetectorPrivate> m_pimpl;
};

#endif // SIBELGENERICDETECTOR_H
