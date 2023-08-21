#ifndef EMPTYDETECTOR_H
#define EMPTYDETECTOR_H

#include <Device/Detector.h>

class EmptyDetector : public Detector
{
    Q_OBJECT
    Q_DISABLE_COPY(EmptyDetector)
public:
    explicit EmptyDetector(QObject *parent = nullptr);
    ~EmptyDetector() override;
    DeviceConfiguration defaultConfiguration() const override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doTestConnection() override;
    bool doPrepare() override;
    bool doCapture() override;
private:
    Frame makeFrame(quint32 lines, float value);
};


#endif // EMPTYDETECTOR_H
