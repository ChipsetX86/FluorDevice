#include "DrgemGxr32PowerSupplyPlugin.h"

#include "DrgemGxr32PowerSupply.h"

DrgemGxr32PowerSupplyPlugin::DrgemGxr32PowerSupplyPlugin(QObject *parent) : QObject(parent)
{

}

DrgemGxr32PowerSupplyPlugin::~DrgemGxr32PowerSupplyPlugin()
{

}

Device *DrgemGxr32PowerSupplyPlugin::create()
{
    return new DrgemGxr32PowerSupply;
}

QString DrgemGxr32PowerSupplyPlugin::description() const
{
    return tr("РПУ Drgem GXR32");
}

QString DrgemGxr32PowerSupplyPlugin::notes() const
{
    return tr("Для работы необходимо установить SDK GXR версии 1.06.32, "
              "драйвер FTDI и настроить номер порта в файле C:\\GXR\\GATEWAY\\CFG\\CONFIG.ini");
}
