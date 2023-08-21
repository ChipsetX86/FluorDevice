#include "EmptyPowerSupplyPlugin.h"

#include "EmptyPowerSupply.h"

EmptyPowerSupplyPlugin::EmptyPowerSupplyPlugin(QObject *parent) : QObject(parent)
{

}

EmptyPowerSupplyPlugin::~EmptyPowerSupplyPlugin()
{

}

Device *EmptyPowerSupplyPlugin::create()
{
    return new EmptyPowerSupply;
}

QString EmptyPowerSupplyPlugin::description() const
{
    return tr("Заглушка для РПУ");
}

QString EmptyPowerSupplyPlugin::notes() const
{
    return QString();
}
