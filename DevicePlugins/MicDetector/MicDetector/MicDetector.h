#ifndef MICDETECTOR_H
#define MICDETECTOR_H

#include <Device/Detector.h>

#include <json_rpc_tcp_client.h>

class MicDetector : public Detector
{
    Q_OBJECT
    Q_DISABLE_COPY(MicDetector)
public:
    explicit MicDetector(QObject *parent = nullptr);
    ~MicDetector() override;
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

    template<typename... T>
    bool callFunctionDetector(QString nameCommand, QVariantMap &responseData, T&&... args);
    bool callFunctionDetector(QString nameCommand);
    bool prepareDetector(int linesCount, qreal chargeTime);
    bool readFrameDetector(Frame &frame);
    bool processResponse(std::shared_ptr<jcon::JsonRpcResult> response,
                            QString nameCommand,
                            QVariantMap &data);
};

#endif // MICDETECTOR_H
