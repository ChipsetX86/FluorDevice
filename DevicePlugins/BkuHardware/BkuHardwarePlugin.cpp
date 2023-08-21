#include "BkuHardwarePlugin.h"

#include "BkuHardware.h"

BkuHardwarePlugin::BkuHardwarePlugin(QObject *parent) : QObject(parent)
{

}

BkuHardwarePlugin::~BkuHardwarePlugin()
{

}

Device *BkuHardwarePlugin::create()
{
    return new BkuHardware;
}

QString BkuHardwarePlugin::description() const
{
    return tr("БКУ с подключением к диспетчеру");
}

QString BkuHardwarePlugin::notes() const
{
    return QString();
}
