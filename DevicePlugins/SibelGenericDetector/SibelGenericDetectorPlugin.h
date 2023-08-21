#ifndef SIBELGENERICDETECTORPLUGIN_H
#define SIBELGENERICDETECTORPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class SibelGenericDetectorPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "SibelGenericDetector.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit SibelGenericDetectorPlugin(QObject *parent = nullptr);
    ~SibelGenericDetectorPlugin() override;

    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // SIBELGENERICDETECTORPLUGIN_H
