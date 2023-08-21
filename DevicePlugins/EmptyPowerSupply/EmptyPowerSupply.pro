include(../PowerSupplyPlugin.pri)
TARGET = $$qtLibraryTarget(EmptyPowerSupply)

HEADERS += \
    EmptyPowerSupply.h \
    EmptyPowerSupplyPlugin.h

SOURCES += \
    EmptyPowerSupply.cpp \
    EmptyPowerSupplyPlugin.cpp

DISTFILES += \
    EmptyPowerSupply.json
