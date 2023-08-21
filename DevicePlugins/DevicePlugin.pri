TEMPLATE = lib
CONFIG   += plugin
QT       += core widgets network

include($$PWD/../Global.pri)
include($$PWD/../Device/Device.pri)

DEVICE_PLUGIN_PATH = $${BINPATH}/DevicePlugins