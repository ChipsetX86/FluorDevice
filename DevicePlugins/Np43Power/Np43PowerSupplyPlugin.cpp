#include "Np43PowerSupplyPlugin.h"

#include "Np43PowerSupply.h"

Np43PowerSupplyPlugin::Np43PowerSupplyPlugin(QObject *parent) : QObject(parent)
{

}

Np43PowerSupplyPlugin::~Np43PowerSupplyPlugin()
{

}

Device *Np43PowerSupplyPlugin::create()
{
    return new Np43PowerSupply;
}

QString Np43PowerSupplyPlugin::description() const
{
    return tr("РПУ производства Научприбор 43 стойка");
}

QString Np43PowerSupplyPlugin::notes() const
{
    return tr("Для корректной работы РПУ необходимо настроить токи накала "
              "в файле указанном в настройках");
}
