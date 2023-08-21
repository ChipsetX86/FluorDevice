#ifndef DEVICEPLUGINMANAGER_H
#define DEVICEPLUGINMANAGER_H

#include "DeviceGlobal.h"

#include <QObject>
#include <QScopedPointer>
#include <QVector>

class Device;

class DEVICELIB_EXPORT DevicePluginManager final
{
    Q_DISABLE_COPY(DevicePluginManager)
public:
    struct PluginInfo
    {
        QString filename;
        QString description;
        QString notes;
    };

    typedef QVector<PluginInfo> PluginInfoList;

    static DevicePluginManager &instance();

    void setBasePath(const QString &path);
    QString basePath() const;

    template <typename T>
    T *create(const QString &filename)
    {
        QScopedPointer<Device> device(createDevice(filename));
        if (auto target = qobject_cast<T *>(device.data())) {
            device.take();
            return target;
        }

        return nullptr;
    }

    Device *createDevice(const QString &filename);

    PluginInfoList lookup(const QString &subPath = QString()) const;
private:
    DevicePluginManager();

    QString m_basePath;
};

#endif // DEVICEPLUGINMANAGER_H
