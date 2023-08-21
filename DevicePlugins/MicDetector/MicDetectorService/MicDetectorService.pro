include($$PWD/../../../Global.pri)
include($$PWD/../../../3rd-party/Jcon-cpp.pri)

QT -= gui
QT += network
CONFIG += console

TARGET = MicDetectorService/$$qtLibraryTarget(MicDetectorService)

SOURCES += \
    DetectorService.cpp \
    MicDetectorWrapper.cpp \
    main.cpp

HEADERS += \
    DetectorService.h \
    MicDetectorWrapper.h \
    mic.h
