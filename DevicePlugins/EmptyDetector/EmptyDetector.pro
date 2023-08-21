include($$PWD/../DetectorPlugin.pri)
TARGET = $$qtLibraryTarget(EmptyDetector)

HEADERS += \
    EmptyDetector.h \
    EmptyDetectorPlugin.h

SOURCES += \
    EmptyDetector.cpp \
    EmptyDetectorPlugin.cpp

DISTFILES += \
    EmptyDetector.json
