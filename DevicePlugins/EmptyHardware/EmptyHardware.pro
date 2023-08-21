include($$PWD/../HardwarePlugin.pri)
TARGET = $$qtLibraryTarget(EmptyHardware)

HEADERS += \
    EmptyHardware.h \
    EmptyHardwarePlugin.h

SOURCES += \
    EmptyHardware.cpp \
    EmptyHardwarePlugin.cpp

DISTFILES += \
    EmptyHardware.json
