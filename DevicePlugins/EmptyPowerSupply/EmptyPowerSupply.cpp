#include "EmptyPowerSupply.h"

EmptyPowerSupply::EmptyPowerSupply(QObject *parent):
    PowerSupply(parent)
{

}

EmptyPowerSupply::~EmptyPowerSupply()
{

}

bool EmptyPowerSupply::doOpen()
{
    return true;
}

void EmptyPowerSupply::doClose()
{

}

bool EmptyPowerSupply::doTestConnection()
{
    return true;
}

bool EmptyPowerSupply::doPrepare()
{
    return true;
}

bool EmptyPowerSupply::doOn()
{
    return true;
}

bool EmptyPowerSupply::doOff()
{
    if (!waitForButtonsReleased()) {
        setLastError(tr("Кнопки не отпущены"));
        return false;
    }

    return true;
}

bool EmptyPowerSupply::doGetResults(Results &results)
{
    const auto params = currentParams();
    results.voltageKV = params.voltageKV;
    results.amperageMA = params.amperageMA;
    return true;
}
