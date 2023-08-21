DEVICE_LIBRARY_FILENAME = $$qtLibraryTarget(Device)

contains(DEFINES, DEVICE_LIBRARY) {
    TARGET = $${DEVICE_LIBRARY_FILENAME}
} else {
    LIBS += -l$${DEVICE_LIBRARY_FILENAME}
}

include($$PWD/../3rd-party/NpApplication.pri)
include($$PWD/../3rd-party/NpToolbox.pri)
include($$PWD/../3rd-party/OpenCV.pri)
include($$PWD/../Settings/Settings.pri)
