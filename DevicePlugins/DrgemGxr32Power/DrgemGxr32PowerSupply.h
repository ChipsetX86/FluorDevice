#ifndef DRGEMGXR32POWERSUPPLY_H
#define DRGEMGXR32POWERSUPPLY_H

#include <Device/PowerSupply.h>

#include <QScopedPointer>

class DrgemGxr32PowerSupply : public PowerSupply
{
    Q_OBJECT
    Q_DISABLE_COPY(DrgemGxr32PowerSupply)
public:
    explicit DrgemGxr32PowerSupply(QObject *parent = nullptr);
    ~DrgemGxr32PowerSupply() override;
    DeviceConfiguration defaultConfiguration() const override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doTestConnection() override;
    bool doPrepare() override;
    bool doOn() override;
    bool doOff() override;
    bool doGetResults(Results &results) override;
    bool doWaitForError() override;
private:
    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif // DRGEMGXR32POWERSUPPLY_H
