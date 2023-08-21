#ifndef SSLDETECTORPLUGIN_H
#define SSLDETECTORPLUGIN_H

#include <QObject>

#include <Device/DevicePlugin.h>

class SslDetectorPlugin : public QObject, public DevicePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID DevicePlugin_IID FILE "SslDetector.json")
    Q_INTERFACES(DevicePlugin)
public:
    explicit SslDetectorPlugin(QObject *parent = nullptr);
    ~SslDetectorPlugin() override;
    Device *create() override;
    QString description() const override;
    QString notes() const override;
};

#endif // SSLDETECTORPLUGIN_H
