include($$PWD/../DetectorPlugin.pri)
include($$PWD/../../3rd-party/Ftdi.pri)
TARGET = $$qtLibraryTarget(SibelGenericDetector)

HEADERS += \
    SibelGenericDetector.h \
    SibelGenericDetectorPlugin.h \
    SibelGenericDetectorPrivate.h

SOURCES += \
    SibelGenericDetector.cpp \
    SibelGenericDetectorPlugin.cpp \
    SibelGenericDetectorPrivate.cpp

DISTFILES += \
    SibelGenericDetector.json
