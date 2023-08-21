#ifndef SSLDETECTOR_H
#define SSLDETECTOR_H

#include <Device/Detector.h>

#include <QScopedPointer>

class SslDetector : public Detector
{
    Q_OBJECT
    Q_DISABLE_COPY(SslDetector)
public:
    explicit SslDetector(QObject *parent = nullptr);
    ~SslDetector() override;
    DeviceConfiguration defaultConfiguration() const override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doTestConnection() override;
    bool doPrepare() override;
    bool doCapture() override;
private:
    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif // SSLDETECTOR_H
