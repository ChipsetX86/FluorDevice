include($$PWD/../../DetectorPlugin.pri)
include($$PWD/../../../3rd-party/Jcon-cpp.pri)
TARGET = $$qtLibraryTarget(MicDetector)

QT += network

HEADERS += \
    MicDetector.h \
    MicDetectorPlugin.h

SOURCES += \
    MicDetector.cpp \
    MicDetectorPlugin.cpp

DISTFILES += \
    MicDetector.json


