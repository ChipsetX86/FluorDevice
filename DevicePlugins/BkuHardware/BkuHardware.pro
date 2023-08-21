include($$PWD/../HardwarePlugin.pri)
TARGET = $$qtLibraryTarget(BkuHardware)

HEADERS += \
    BkuHardware.h \
    BkuHardwarePlugin.h

SOURCES += \
    BkuHardware.cpp \
    BkuHardwarePlugin.cpp

DISTFILES += \
    BkuHardware.json
