#ifndef DRGEMGXR32POWERSUPPLYPLUGIN_H
#define DRGEMGXR32POWERSUPPLYPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class DrgemGxr32PowerSupplyPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "DrgemGxr32Power.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit DrgemGxr32PowerSupplyPlugin(QObject *parent = nullptr);
    ~DrgemGxr32PowerSupplyPlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // DRGEMGXR32POWERSUPPLYPLUGIN_H
