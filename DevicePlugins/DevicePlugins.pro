TEMPLATE = subdirs
SUBDIRS += EmptyDetector \
    EmptyHardware \
    EmptyPowerSupply \
    BkuHardware \
    IstraMonoPower \
    Np43Power \
    SibelGenericDetector \
    SslDetector \
    DrgemGxr32Power

win32 {
    SUBDIRS += MicDetector
}
