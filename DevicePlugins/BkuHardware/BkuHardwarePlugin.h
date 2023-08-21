#ifndef BKUHARDWAREPLUGIN_H
#define BKUHARDWAREPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class BkuHardwarePlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "BkuHardware.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit BkuHardwarePlugin(QObject *parent = nullptr);
    ~BkuHardwarePlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // BKUHARDWAREPLUGIN_H
