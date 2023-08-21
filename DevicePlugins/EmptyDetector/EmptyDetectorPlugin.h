#ifndef EMPTYDETECTORPLUGIN_H
#define EMPTYDETECTORPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class EmptyDetectorPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "EmptyDetector.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit EmptyDetectorPlugin(QObject *parent = nullptr);
    ~EmptyDetectorPlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // EMPTYDETECTORPLUGIN_H
