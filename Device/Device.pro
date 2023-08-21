include($$PWD/../Global.pri)

DEFINES  += DEVICE_LIBRARY

QT       -= gui
QT       += core serialport
CONFIG   += dll
TEMPLATE  = lib

include($$PWD/Device.pri)

HEADERS += Scanner.h \
    BinningAcquisitionResultProcessor.h \
    CancelationToken.h \
    Detector.h \
    Device.h \
    DeviceConfiguration.h \
    DeviceGlobal.h \
    DeviceLogging.h \
    DevicePlugin.h \
    DevicePluginManager.h \
    Dispatcher.h \
    FlipAcquisitionResultProcessor.h \
    Hardware.h \
    NpFrame.h \
    PowerSupply.h \
    FlatFieldCorrection.h \
    ScaleAcquisitionResultProcessor.h \
    ScannerAcquisitionResultProcessor.h \
    ScannerCalibrationData.h \
    ScanningModesCollection.h

SOURCES += Scanner.cpp \
    BinningAcquisitionResultProcessor.cpp \
    CancelationToken.cpp \
    Device.cpp \
    DeviceConfiguration.cpp \
    DeviceLogging.cpp \
    DevicePlugin.cpp \
    DevicePluginManager.cpp \
    Detector.cpp \
    Dispatcher.cpp \
    FlipAcquisitionResultProcessor.cpp \
    Hardware.cpp \
    NpFrame.cpp \
    PowerSupply.cpp \
    FlatFieldCorrection.cpp \
    ScaleAcquisitionResultProcessor.cpp \
    ScannerAcquisitionResultProcessor.cpp \
    ScannerCalibrationData.cpp \
    ScanningModesCollection.cpp
