#include "DevicePluginManager.h"

#include <QCoreApplication>
#include <QPluginLoader>
#include <QDir>
#include <QDirIterator>
#include <QThread>

#include <NpApplication/Application.h>
#include <NpToolbox/Toolbox.h>

#include "DeviceLogging.h"
#include "Device.h"
#include "DevicePlugin.h"

using namespace Nauchpribor;

namespace {
    DevicePlugin *loadDevicePlugin(const QString &filename)
    {
        QPluginLoader loader(filename);
        if (loader.load()) {
            if (auto plugin = dynamic_cast<DevicePlugin *>(loader.instance())) {
                const QString programTranslationsPath =
                              QCoreApplication::applicationDirPath() + QStringLiteral("/") +
                              TRANSLATIONS_PATH_SUFFIX;
                
                const QString translation = Toolbox::fromCamelCase(loader.instance()->metaObject()->className());
                if (!translation.isEmpty()) {
                    if (QThread::currentThread() == npApp->instance()->thread()) {
                        npApp->loadTranslation(translation, programTranslationsPath);
                    } else {
                        QMetaObject::invokeMethod(npApp, [translation, programTranslationsPath] {
                            npApp->loadTranslation(translation, programTranslationsPath);
                        },  Qt::BlockingQueuedConnection);
                    }
                }

                return plugin;
            } else {
                errDevice << "Plugin is not DevicePlugin:" << filename;
            }
        } else {
            errDevice << QStringLiteral("Plugin is not loaded: %1 (%2)").arg(filename, loader.errorString());
        }

        return nullptr;
    }
}

DevicePluginManager &DevicePluginManager::instance()
{
    static DevicePluginManager instance;
    return instance;
}

void DevicePluginManager::setBasePath(const QString &path)
{
    QDir dir(path);
    m_basePath = dir.path();

    dbgDevice << "Change base path to" << m_basePath;
}

QString DevicePluginManager::basePath() const
{
    return m_basePath;
}

DevicePluginManager::PluginInfoList DevicePluginManager::lookup(const QString &subPath) const
{
    static const QStringList extensions = {
        QStringLiteral("*.dll"),
        QStringLiteral("*.so")
    };

    QString path = m_basePath;
    if (!subPath.isEmpty()) {
        path += QStringLiteral("/") + subPath;
    }

    QDir dir(path);

    infoDevice << "Lookup for plugins:" << dir.path();

    PluginInfoList result;

    const QDir baseDir(m_basePath);
    QDirIterator it(dir.path(), extensions, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString pluginFilename = it.next();
        if (auto plugin = loadDevicePlugin(pluginFilename)) {
            PluginInfo info;
            info.filename = baseDir.relativeFilePath(pluginFilename);
            info.description = plugin->description();
            info.notes = plugin->notes();
            result.append(info);
        }
    }

    return result;
}

DevicePluginManager::DevicePluginManager() :
    m_basePath(QCoreApplication::applicationDirPath() + QStringLiteral("/DevicePlugins"))
{

}

Device *DevicePluginManager::createDevice(const QString &filename)
{
    if (!filename.isEmpty()) {
        const QString pluginFilename = m_basePath + QStringLiteral("/") + filename;
        if (auto plugin = loadDevicePlugin(pluginFilename)) {
            return plugin->create();
        }
    }

    return nullptr;
}
