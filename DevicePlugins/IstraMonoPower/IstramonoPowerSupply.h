#ifndef ISTRAMONOPOWERSUPPLY_H
#define ISTRAMONOPOWERSUPPLY_H

#include <Device/PowerSupply.h>
#include <Device/Dispatcher.h>

class IstramonoPowerSupply : public PowerSupply
{
    Q_OBJECT
    Q_DISABLE_COPY(IstramonoPowerSupply)
public:
    explicit IstramonoPowerSupply(QObject *parent = nullptr);
    ~IstramonoPowerSupply() override;
    DeviceConfiguration defaultConfiguration() const override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doReset() override;
    bool doTestConnection() override;
    bool doPrepare() override;
    bool doOn() override;
    bool doWaitForError() override;
    bool doOff() override;
    bool doGetResults(Results &results) override;
private:
    QString decodeError(int errCode) const;

    Dispatcher::Accessor *m_accessor;
};

#endif // ISTRAMONOPOWERSUPPLY_H
