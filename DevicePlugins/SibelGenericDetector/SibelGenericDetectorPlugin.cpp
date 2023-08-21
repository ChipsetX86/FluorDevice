#include "SibelGenericDetectorPlugin.h"

#include "SibelGenericDetector.h"

SibelGenericDetectorPlugin::SibelGenericDetectorPlugin(QObject *parent) : QObject(parent)
{

}

SibelGenericDetectorPlugin::~SibelGenericDetectorPlugin()
{

}

Device *SibelGenericDetectorPlugin::create()
{
    return new SibelGenericDetector;
}

QString SibelGenericDetectorPlugin::description() const
{
    return tr("Сибел 2048/4096/4608");
}

QString SibelGenericDetectorPlugin::notes() const
{
    return tr("Для работы детектора необходимо установка драйвер FTDI");
}
