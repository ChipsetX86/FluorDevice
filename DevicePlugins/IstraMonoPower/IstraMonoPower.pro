include(../PowerSupplyPlugin.pri)
TARGET = $$qtLibraryTarget(IstramonoPower)

HEADERS += \
    IstramonoPowerSupply.h \
    IstramonoPowerSupplyPlugin.h

SOURCES += \
    IstramonoPowerSupply.cpp \
    IstramonoPowerSupplyPlugin.cpp

DISTFILES += \
    IstramonoPower.json
