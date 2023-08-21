#include "MicDetectorPlugin.h"

#include "MicDetector.h"

MicDetectorPlugin::MicDetectorPlugin(QObject *parent) : QObject(parent)
{

}

MicDetectorPlugin::~MicDetectorPlugin()
{

}

Device *MicDetectorPlugin::create()
{
    return new MicDetector;
}

QString MicDetectorPlugin::description() const
{
    return tr("МИК (ионизационная камера) подключение через Ethernet");
}

QString MicDetectorPlugin::notes() const
{
    return tr("Для работы детектора необходимо выбрать нужную сетевую плату "
              "в программе netshow и установить нужный драйвер в папке MicDetectorService (см. ReadMe.txt)");
}
