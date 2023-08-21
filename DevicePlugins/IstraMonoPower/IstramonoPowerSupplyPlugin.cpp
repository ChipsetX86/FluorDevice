#include "IstramonoPowerSupplyPlugin.h"

#include "IstramonoPowerSupply.h"

IstramonoPowerSupplyPlugin::IstramonoPowerSupplyPlugin(QObject *parent) : QObject(parent)
{

}

IstramonoPowerSupplyPlugin::~IstramonoPowerSupplyPlugin()
{

}

Device *IstramonoPowerSupplyPlugin::create()
{
    return new IstramonoPowerSupply;
}

QString IstramonoPowerSupplyPlugin::description() const
{
    return tr("РПУ Истра");
}

QString IstramonoPowerSupplyPlugin::notes() const
{
    return tr("Для работы необходимо настроить переключатели на плате РПУ в "
              "режим работы по 485 протоколу, прошить МК версией прошивки MC_Fluoro_V351_Orel.bin "
              "или MC_Fluoro_V721_Orel в зависимости от исполнения и провести калибровку РПУ через ПО Pult_URP_Orel_v406");
}
