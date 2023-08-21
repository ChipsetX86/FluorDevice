#ifndef ISTRAMONOPOWERSUPPLYPLUGIN_H
#define ISTRAMONOPOWERSUPPLYPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class IstramonoPowerSupplyPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "IstraMonoPower.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit IstramonoPowerSupplyPlugin(QObject *parent = nullptr);
    ~IstramonoPowerSupplyPlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // ISTRAMONOPOWERSUPPLYPLUGIN_H
