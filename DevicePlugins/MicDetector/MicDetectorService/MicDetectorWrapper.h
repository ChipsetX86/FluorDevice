#ifndef MICDETECTORWRAPPER_H
#define MICDETECTORWRAPPER_H

#include <QObject>

#include "../Common.h"

class MicDetectorWrapper: public QObject
{
    Q_OBJECT
public:
    struct HardwareInfo
    {
        ushort widthPx;
    };

    explicit MicDetectorWrapper(QObject *parent = nullptr);
    bool loadMicLibrary();
    bool hardwareInit();
    bool readHardwareInfo(HardwareInfo &info);
    bool hardwareShutdown();
    bool prepareReading(const ushort linesCount, const float chargeTimeMs);
    bool readData(MicFrame &frame);
    QString lastError();
private:
    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif // MICDETECTORWRAPPER_H
