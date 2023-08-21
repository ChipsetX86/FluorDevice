#ifndef DEVICEPLUGIN_H
#define DEVICEPLUGIN_H

#include "DeviceGlobal.h"

#include <QtPlugin>

class Device;

class DEVICELIB_EXPORT DevicePlugin
{
public:    
    virtual ~DevicePlugin();
    virtual Device *create() = 0;
    virtual QString description() const = 0;
    virtual QString notes() const = 0;
};

#define DevicePlugin_IID "ru.nauchpribor.hardware.deviceplugin/1.0"

Q_DECLARE_INTERFACE(DevicePlugin, DevicePlugin_IID)

#endif // DEVICEPLUGIN_H
