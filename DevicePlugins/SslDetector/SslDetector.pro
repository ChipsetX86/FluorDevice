include($$PWD/../DetectorPlugin.pri)
TARGET = $$qtLibraryTarget(SslDetector)

HEADERS += \
    SslDetector.h \
    SslDetectorPlugin.h

SOURCES += \
    SslDetector.cpp \
    SslDetectorPlugin.cpp

DISTFILES += \
    SslDetector.json

OTHER_FILES += dll_src.h
