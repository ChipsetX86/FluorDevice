#ifndef EMPTYHARDWAREPLUGIN_H
#define EMPTYHARDWAREPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class EmptyHardwarePlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "EmptyHardware.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit EmptyHardwarePlugin(QObject *parent = nullptr);
    ~EmptyHardwarePlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // EMPTYHARDWAREPLUGIN_H
