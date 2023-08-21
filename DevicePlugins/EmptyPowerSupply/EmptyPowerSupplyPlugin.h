#ifndef EMPTYPOWERSUPPLYPLUGIN_H
#define EMPTYPOWERSUPPLYPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class EmptyPowerSupplyPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "EmptyPowerSupply.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit EmptyPowerSupplyPlugin(QObject *parent = nullptr);
    ~EmptyPowerSupplyPlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // EMPTYPOWERSUPPLYPLUGIN_H
