#ifndef NP43POWERSUPPLYPLUGIN_H
#define NP43POWERSUPPLYPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class Np43PowerSupplyPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "Np43Power.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit Np43PowerSupplyPlugin(QObject *parent = nullptr);
    ~Np43PowerSupplyPlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // NP43POWERSUPPLYPLUGIN_H
