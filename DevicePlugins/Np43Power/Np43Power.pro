include(../PowerSupplyPlugin.pri)
include(../../Settings/Settings.pri)
TARGET = $$qtLibraryTarget(Np43Power)

DISTFILES += \
    Np43Power.json

HEADERS += \
    Np43.h \
    Np43PowerSupply.h \
    Np43PowerSupplyPlugin.h

SOURCES += \
    Np43PowerSupply.cpp \
    Np43PowerSupplyPlugin.cpp

RESOURCES += Resorces.qrc
