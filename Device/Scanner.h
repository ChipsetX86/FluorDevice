#ifndef SCANNER_H
#define SCANNER_H

#include "DeviceGlobal.h"

#include <QObject>
#include <QMap>
#include <QScopedPointer>
#include <QSizeF>
#include <QVector>
#include <QDateTime>
#include <QThread>

#include <NpToolbox/Atomic.h>

#include "ScanningModesCollection.h"
#include "CancelationToken.h"

class Device;
class Detector;
class Hardware;
class PowerSupply;
class Dispatcher;
class QSettings;
class QTimer;

class DEVICELIB_EXPORT Scanner final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Scanner)
public:
    enum State {
        Unknown = -1,
        Idle,
        Prepare,
        Calibration,
        Acquisition,
        Finalization,
        Overheat,
        Error
    };

    Q_ENUM(State)

    static const QString detectorPluginsSubPath;
    static const QString powerSupplyPluginsSubPath;
    static const QString hardwarePluginsSubPath;

    struct AcquisitionResult
    {
        // Don't forget fix QDataStream operators
        // if this struct changed
        double amperageMa;
        double voltageKv;
        quint16 exposureMs;
        int width;
        QVector<float> image;
        QVector<float> dark;
        QSizeF pixelSize;
    };

    struct AcquisitionParams
    {
        ScanningModesCollection::Item scanningMode;
        quint16 heightMm;
        double voltageKv;
        double amperageMa;
        bool useDoor;
    };

    explicit Scanner(QObject *parent = nullptr);
    ~Scanner() override;

    static Scanner *instance();

    QString lastError() const;
    AcquisitionResult lastAcquisitionResult() const;

    bool isXRayOn() const;
    Scanner::State state() const;

    quint32 secsToCooldown() const;
public slots:
    bool open();
    void close();
    bool reset();

    bool makeAcquisition(const Scanner::AcquisitionParams &params);
    bool makeCalibration();

    void moveRackUp();
    void moveRackDown();
    void openFirstDoor(CancelationTokenSource::Token token);
    void closeFirstDoor(CancelationTokenSource::Token token);
    void stop();
signals:
    void opened();
    void closed();
    void errorOccurred(QString error);
    void stateChanged(Scanner::State state);

    void xrayToggled(bool value);
    void acquisitionResultReady(Scanner::AcquisitionResult result);
    void calibrationProgress(int current, int total);
private:
    bool configureDispatcher();
    bool configureDetector();
    bool configurePowerSupply();
    bool configureHardware();

    void dismissDispatcher();
    void dismissDetector();
    void dismissPowerSupply();
    void dismissHardware();

    bool openDevices();
    bool resetDevices();
    void closeDevices();
    bool pingDevices();

    bool checkIsOpen();
    void setLastError(const QString &error);
    void setState(Scanner::State state);
    void processAcqusitionResult(const ScanningModesCollection::Item &scanningMode);

    bool switchDevicesConfigurations(const ScanningModesCollection::Item &scanningMode);
    void accumulateReleasedPower(double kV, double mA, ushort exposureMs);

    static Scanner *m_lastCreatedScanner;

    QVector<Device *> devices() const;
    QString deviceLastError(Device *device);

    QSettings *m_run;

    Nauchpribor::Toolbox::Atomic<State> m_state;
    Nauchpribor::Toolbox::Atomic<bool> m_isXrayOn;
    Nauchpribor::Toolbox::Atomic<QString> m_lastError;
    Nauchpribor::Toolbox::Atomic<AcquisitionResult> m_lastAcquisitionResult;
    AcquisitionResult m_currentAcquisitionResult;

    Dispatcher *m_dispatcher;
    Detector *m_detector;
    PowerSupply *m_powerSupply;
    Hardware *m_hardware;

    QThread m_dispatcherThread;
    QThread m_detectorThread;
    QThread m_hardwareThread;
    QThread m_powerSupplyThread;

    QDateTime m_cooldownDateTime;
    QTimer *m_overheatTimer;
};

DEVICELIB_EXPORT QDataStream &operator<<(QDataStream &stream, const Scanner::AcquisitionResult &result);
DEVICELIB_EXPORT QDataStream &operator>>(QDataStream &stream, Scanner::AcquisitionResult &result);

Q_DECLARE_METATYPE(Scanner::State)
Q_DECLARE_METATYPE(Scanner::AcquisitionResult)

#endif
