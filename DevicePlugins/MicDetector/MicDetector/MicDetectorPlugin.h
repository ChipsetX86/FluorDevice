#ifndef MICDETECTORPLUGIN_H
#define MICDETECTORPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class MicDetectorPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "MicDetector.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit MicDetectorPlugin(QObject *parent = nullptr);
    ~MicDetectorPlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // MICDETECTORPLUGIN_H
