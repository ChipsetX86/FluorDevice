#include "EmptyDetectorPlugin.h"

#include "EmptyDetector.h"

EmptyDetectorPlugin::EmptyDetectorPlugin(QObject *parent) : QObject(parent)
{

}

EmptyDetectorPlugin::~EmptyDetectorPlugin()
{

}

Device *EmptyDetectorPlugin::create()
{
    return new EmptyDetector;
}

QString EmptyDetectorPlugin::description() const
{
    return tr("Заглушка для детектора");
}

QString EmptyDetectorPlugin::notes() const
{
    return QString();
}
