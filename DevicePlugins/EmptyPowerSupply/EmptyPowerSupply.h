#ifndef EMPTYPOWERSUPPLY_H
#define EMPTYPOWERSUPPLY_H

#include <Device/PowerSupply.h>

class EmptyPowerSupply : public PowerSupply
{
    Q_OBJECT
    Q_DISABLE_COPY(EmptyPowerSupply)
public:
    explicit EmptyPowerSupply(QObject *parent = nullptr);
    ~EmptyPowerSupply() override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doTestConnection() override;
    bool doPrepare() override;
    bool doOn() override;
    bool doOff() override;
    bool doGetResults(Results &results) override;
};

#endif // EMPTYPOWERSUPPLY_H
