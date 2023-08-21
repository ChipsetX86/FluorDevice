include(../PowerSupplyPlugin.pri)
TARGET = $$qtLibraryTarget(DrgemGxr32Power)

DISTFILES += \
    DrgemGxr32Power.json

HEADERS += \
    DrgemGxr32PowerSupply.h \
    DrgemGxr32PowerSupplyPlugin.h \
    Gxr.h \
    GxrPackage.h

SOURCES += \
    DrgemGxr32PowerSupply.cpp \
    DrgemGxr32PowerSupplyPlugin.cpp \
    GxrPackage.cpp
