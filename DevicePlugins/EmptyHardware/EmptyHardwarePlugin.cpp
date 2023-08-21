#include "EmptyHardwarePlugin.h"

#include "EmptyHardware.h"

EmptyHardwarePlugin::EmptyHardwarePlugin(QObject *parent) : QObject(parent)
{

}

EmptyHardwarePlugin::~EmptyHardwarePlugin()
{

}

Device *EmptyHardwarePlugin::create()
{
    return new EmptyHardware;
}

QString EmptyHardwarePlugin::description() const
{
    return tr("Заглушка для механики");
}

QString EmptyHardwarePlugin::notes() const
{
    return QString();
}
