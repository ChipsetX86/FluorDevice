TEMPLATE = subdirs

win32 {
    equals(QMAKE_TARGET.arch, x86) {
        SUBDIRS += MicDetectorService
    }

    SUBDIRS += MicDetector
}
