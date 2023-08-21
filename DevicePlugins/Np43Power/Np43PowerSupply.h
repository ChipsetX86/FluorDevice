#ifndef NP43POWERSUPPLY_H
#define NP43POWERSUPPLY_H

#include <Device/PowerSupply.h>

class Np43PowerSupply: public PowerSupply
{
    Q_OBJECT
    Q_DISABLE_COPY(Np43PowerSupply)
public:
    explicit Np43PowerSupply(QObject *parent = nullptr);
    ~Np43PowerSupply() override;
    DeviceConfiguration defaultConfiguration() const override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doTestConnection() override;
    bool doPrepare() override;
    bool doOn() override;
    bool doWaitForError() override;
    bool doOff() override;
    bool doGetResults(Results &results) override;
private:
    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif // NP43POWERSUPPLY_H
