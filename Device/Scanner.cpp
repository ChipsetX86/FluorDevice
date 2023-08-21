#include "Scanner.h"

#include <QElapsedTimer>
#include <QtMath>
#include <QSettings>
#include <QTimer>
#include <QDataStream>

#include <NpApplication/Application.h>
#include <NpToolbox/Invoker.h>
#include <Settings/LocalSettings.h>

#include "Dispatcher.h"
#include "Detector.h"
#include "Hardware.h"
#include "PowerSupply.h"
#include "DeviceConfiguration.h"
#include "DevicePluginManager.h"
#include "ScannerCalibrationData.h"
#include "DeviceLogging.h"

using namespace Nauchpribor;

const QString Scanner::detectorPluginsSubPath = QStringLiteral("Detector");
const QString Scanner::powerSupplyPluginsSubPath = QStringLiteral("PowerSupply");
const QString Scanner::hardwarePluginsSubPath = QStringLiteral("Hardware");

namespace {
    static const int qmtState = qRegisterMetaType<Scanner::State>();
    static const int qmtAcquisitionResult = qRegisterMetaType<Scanner::AcquisitionResult>();

    // Delay for protect Omron from crazy
    // while direction changed from up to down
    const uint OMRON_DELAY_MS = 500;
    const quint16 DARK_FRAME_HEIGHT_MM = 200;
    const quint16 CALIBRATION_HEIGHT_MM = 400;

    const QString kCooldownTimeParam = QStringLiteral("main/cooldown");

    quint32 calculateDetectorLines(quint16 heightMm, float pixelHeightMm)
    {
        return pixelHeightMm > 0 ? qCeil(heightMm / pixelHeightMm) : 0;
    }

    quint16 calculateExposureTime(quint32 lines, float chargeTimeMs)
    {
        return qCeil(lines * chargeTimeMs);
    }

    void quitAndWaitThread(QThread &thread)
    {
        thread.quit();
        thread.wait();
    }
}

Scanner *Scanner::m_lastCreatedScanner = nullptr;

Scanner::Scanner(QObject *parent) : QObject(parent),
    m_state(State::Unknown),
    m_isXrayOn(false),
    m_dispatcher(nullptr),
    m_detector(nullptr),
    m_powerSupply(nullptr),
    m_hardware(nullptr),
    m_overheatTimer(new QTimer(this))
{
    m_lastCreatedScanner = this;

    m_run = new QSettings(npApp->permanentDataFilename(QStringLiteral("scanner.run")),
                                                       QSettings::IniFormat, this);

    m_cooldownDateTime = m_run->value(kCooldownTimeParam).toDateTime();

    connect(m_overheatTimer, &QTimer::timeout, this, [this] {
        if (state() == State::Overheat) {
            setState(State::Idle);
        }
    });

    m_overheatTimer->setInterval(1000);
}

Scanner::~Scanner()
{
    close();

    if (m_lastCreatedScanner == this) {
        m_lastCreatedScanner = nullptr;
    }
}

Scanner *Scanner::instance()
{
    return m_lastCreatedScanner;
}

bool Scanner::configureDetector()
{
    if (m_detector) {
        warnDevice << "Detector already configured";
        return true;
    }

    QString filename = LocalSettings::instance().scannerDetectorPlugin();    
    m_detector = DevicePluginManager::instance().create<Detector>(filename);

    if (!m_detector) {
        setLastError(tr("Плагин детектора не загружен"));
        return false;
    }

    infoDevice << "Using detector plugin:" << filename;

    m_detectorThread.start(QThread::HighPriority);

    m_detector->setDispatcher(m_dispatcher);
    m_detector->moveToThread(&m_detectorThread);
    connect(&m_detectorThread, &QThread::finished, m_detector, &QObject::deleteLater);

    return true;
}

bool Scanner::configurePowerSupply()
{
    if (m_powerSupply) {
        warnDevice << "Power supply already configured";
        return true;
    }

    QString filename = LocalSettings::instance().scannerPowerSupplyPlugin();    
    m_powerSupply = DevicePluginManager::instance().create<PowerSupply>(filename);

    if (!m_powerSupply) {
        setLastError(tr("Плагин РПУ не загружен"));
        return false;
    }

    infoDevice << "Using power supply plugin:" << filename;

    m_powerSupplyThread.start(QThread::HighPriority);

    m_powerSupply->setDispatcher(m_dispatcher);
    m_powerSupply->moveToThread(&m_powerSupplyThread);
    connect(m_powerSupply, &PowerSupply::toggled, this, &Scanner::xrayToggled, Qt::DirectConnection);
    connect(&m_powerSupplyThread, &QThread::finished, m_powerSupply, &QObject::deleteLater);    

    return true;
}

bool Scanner::configureHardware()
{
    if (m_hardware) {
        warnDevice << "Hardware already configured";
        return true;
    }

    QString filename = LocalSettings::instance().scannerHardwarePlugin();    
    m_hardware = DevicePluginManager::instance().create<Hardware>(filename);

    if (!m_hardware) {
        setLastError(tr("Плагин механики не загружен"));
        return false;
    }

    infoDevice << "Using hardware plugin:" << filename;

    m_hardwareThread.start(QThread::HighPriority);

    m_hardware->setDispatcher(m_dispatcher);
    m_hardware->moveToThread(&m_hardwareThread);
    connect(&m_hardwareThread, &QThread::finished, m_hardware, &QObject::deleteLater);

    return true;
}

void Scanner::dismissDispatcher()
{
    quitAndWaitThread(m_dispatcherThread);
    m_dispatcher = nullptr;
}

void Scanner::dismissDetector()
{
    quitAndWaitThread(m_detectorThread);
    m_detector = nullptr;
}

void Scanner::dismissPowerSupply()
{
    quitAndWaitThread(m_powerSupplyThread);
    m_powerSupply = nullptr;
}

void Scanner::dismissHardware()
{
    quitAndWaitThread(m_hardwareThread);
    m_hardware = nullptr;
}

bool Scanner::pingDevices()
{
    QMap<Device *, Toolbox::Invoker::Outcome<bool>> outcomes;

    for (auto &device : devices()) {
        if (device) {
            outcomes.insert(device, Toolbox::Invoker::run(device, &Device::testConnection));
        }
    }

    bool success = true;

    for (auto it = outcomes.cbegin(); it != outcomes.cend(); ++it) {
        if (!it.value().result() && success) {
            setLastError(deviceLastError(it.key()));
            success = false;
        }
    }

    return success;
}

void Scanner::setLastError(const QString &error)
{
    m_lastError = error;
    emit errorOccurred(error);
}

void Scanner::setState(Scanner::State state)
{
    if (state == State::Idle && secsToCooldown()) {
        state = State::Overheat;
    }

    if (m_state != state) {
        dbgDevice << "Change scanner state from" << m_state.load() << "to" << state;
        m_state = state;
        emit stateChanged(state);

        switch (m_state) {
        case State::Overheat:
            m_overheatTimer->start();
            break;
        default:
            m_overheatTimer->stop();
        }
    }
}

bool Scanner::open()
{
    if (m_state != State::Unknown) {
        setLastError(tr("Уже инициализирован"));
        return false;
    }

    if (!configureDispatcher() ||
        !configureDetector() ||
        !configurePowerSupply() ||
        !configureHardware()) {
        return false;
    }

    if (!openDevices()) {
        return false;
    }

    emit opened();
    setState(State::Idle);
    return true;
}

void Scanner::close()
{
    closeDevices();

    dismissHardware();
    dismissPowerSupply();
    dismissDetector();
    dismissDispatcher();

    if (m_state != State::Unknown) {
        emit closed();
        setState(State::Unknown);
    }
}

bool Scanner::makeAcquisition(const AcquisitionParams &params)
{
    m_currentAcquisitionResult = AcquisitionResult();

    auto &settings = LocalSettings::instance();

    if (m_state != State::Idle) {
        setLastError(tr("Флюорограф не готов"));
        return false;
    }

    if (!switchDevicesConfigurations(params.scanningMode)) {
        return false;
    }

    const quint8 doorsCount = settings.scannerDoorsCount();

    Detector::Properties detectorProperties = m_detector->properties();

    if (settings.scannerFlatFieldCorrectionEnabled() && ScannerCalibrationData::instance().updateIsRequired()) {
        setLastError(tr("Калибровочные коэффициенты устарели. Выполните калибровку"));
        return false;
    }

    const auto linesCount = calculateDetectorLines(params.heightMm, detectorProperties.pixelSizeMm.height());
    const auto darkLinesCount = calculateDetectorLines(DARK_FRAME_HEIGHT_MM, detectorProperties.pixelSizeMm.height());
    const auto exposureTimeMs = calculateExposureTime(linesCount, detectorProperties.chargeTimeMsec);

    m_currentAcquisitionResult.width = detectorProperties.width;
    m_currentAcquisitionResult.pixelSize = detectorProperties.pixelSizeMm;

    qint64 rollbackTimeMs = 0;

    {
        setState(State::Prepare);

        if (!pingDevices()) {
            setState(State::Error);
            return false;
        }

        if (!Toolbox::Invoker::run(m_hardware, &Hardware::refreshState).result()) {
            setLastError(tr("Не удалось получить статус механики. %1").arg(m_hardware->lastError()));
            setState(State::Error);
            return false;
        }

        if (m_hardware->rackState() == Hardware::RackStateTop) {
            setLastError(tr("Механика находится в крайнем верхнем положении"));
            setState(State::Error);
            return false;
        }

        if (!Toolbox::Invoker::run(m_hardware, &Hardware::lockRemote).result()) {
            setLastError(tr("Не удалось разблокировать пульт. %1").arg(m_hardware->lastError()));
            setState(State::Error);
            return false;
        }

        Toolbox::Invoker::Outcome<bool> hardwareOutcome;
        if (params.useDoor && doorsCount >= 1) {
            hardwareOutcome = Toolbox::Invoker::run(m_hardware, &Hardware::closeFirstDoor);
        } else {
            hardwareOutcome.setResult(true);
        }

        Toolbox::Invoker::Outcome<bool> powerSupplyOutcome;

        {
            PowerSupply::Params powerSupplyParams;
            powerSupplyParams.voltageKV = params.voltageKv;
            powerSupplyParams.amperageMA = params.amperageMa;
            powerSupplyParams.exposureMs = exposureTimeMs;

            powerSupplyOutcome = Toolbox::Invoker::run(m_powerSupply, &PowerSupply::prepare, powerSupplyParams);
        }

        Toolbox::Invoker::run(m_detector, &Detector::prepare, darkLinesCount).waitForFinished();
        auto detectorOutcome = Toolbox::Invoker::run(m_detector, &Detector::capture);

        if (detectorOutcome.result()) {
            detectorOutcome = Toolbox::Invoker::run(m_detector, &Detector::prepare, linesCount);
        }

        detectorOutcome.waitForFinished();
        hardwareOutcome.waitForFinished();
        powerSupplyOutcome.waitForFinished();

        if (!detectorOutcome.result()) {
            setLastError(tr("Не удалось выполнить подготовку детектора. %1").arg(m_detector->lastError()));
            setState(State::Error);
            return false;
        }

        if (!hardwareOutcome.result()) {
            setLastError(tr("Не удалось выполнить подготовку механики. %1").arg(m_hardware->lastError()));
            setState(State::Error);
            return false;
        }

        if (!powerSupplyOutcome.result()) {
            setLastError(tr("Не удалось выполнить подготовку РПУ. %1").arg(m_powerSupply->lastError()));
            setState(State::Error);
            return false;
        }

        if (!Toolbox::Invoker::run(m_hardware, &Hardware::pressPrepareButton).result()) {
            setLastError(tr("Не удалось нажать кнопку подготовки. %1").arg(m_hardware->lastError()));
            setState(State::Error);
            return false;
        }

        m_currentAcquisitionResult.dark = m_detector->lastCapturedFrame();
    }

    {
        QThread::msleep(settings.scannerDelayBeforeScanMs());

        bool fatalErrorOccurred = false;

        QElapsedTimer movingTimer;

        setState(State::Acquisition);

        if (!Toolbox::Invoker::run(m_hardware, &Hardware::startScan, m_powerSupply).result()) {
            setLastError(tr("Не удалось начать движение механики. %1").arg(m_hardware->lastError()));
            setState(State::Error);
            return false;
        }

        movingTimer.start();

        auto detectorOutcome = Toolbox::Invoker::runDelayed(settings.scannerDetectorDelayMs(), m_detector, &Detector::capture);
        auto powerSupplyOutcome = Toolbox::Invoker::run(m_powerSupply, &PowerSupply::launch);

        if (!detectorOutcome.result()) {
            setLastError(tr("Не удалось получить изображение с детектора. %1").arg(m_detector->lastError()));
            fatalErrorOccurred = true;
        } else {
            m_currentAcquisitionResult.image = m_detector->lastCapturedFrame();
        }

        if (!Toolbox::Invoker::run(m_hardware, &Hardware::stopScan).result()) {
            setLastError(tr("Не удалось остановить механику. %1").arg(m_hardware->lastError()));
            fatalErrorOccurred = true;
        }

        rollbackTimeMs = movingTimer.elapsed();
        if (rollbackTimeMs > params.scanningMode.rollbackAlpha) {
            rollbackTimeMs -= params.scanningMode.rollbackAlpha;
        }

        rollbackTimeMs *= params.scanningMode.rollbackBeta;

        if (!powerSupplyOutcome.result()) {
            setLastError(tr("Ошибка включения-выключения РПУ. %1").arg(m_powerSupply->lastError()));
            fatalErrorOccurred = true;
        }

        if (!Toolbox::Invoker::run(m_powerSupply, &PowerSupply::getResults).result()) {
            if (!fatalErrorOccurred) {
                setLastError(tr("Не удалось получить результаты работы РПУ. %1").arg(m_powerSupply->lastError()));
            }

            accumulateReleasedPower(params.voltageKv, params.amperageMa, exposureTimeMs);
        } else {
            PowerSupply::Results powerResults = m_powerSupply->results();

            m_currentAcquisitionResult.amperageMa = powerResults.amperageMA;
            m_currentAcquisitionResult.voltageKv = powerResults.voltageKV;
            m_currentAcquisitionResult.exposureMs = powerResults.exposureMs;

            accumulateReleasedPower(m_currentAcquisitionResult.voltageKv,
                                    m_currentAcquisitionResult.amperageMa,
                                    m_currentAcquisitionResult.exposureMs);
        }

        processAcqusitionResult(params.scanningMode);

        if (fatalErrorOccurred) {
            setState(State::Error);
            return false;
        }
    }

    {
        setState(State::Finalization);

        if (params.useDoor && doorsCount >= 1) {
            if (!Toolbox::Invoker::run(m_hardware, &Hardware::openFirstDoor).result()) {
                setLastError(tr("Не удалось открыть дверь"));
                setState(State::Error);
                return false;
            }
        } else {
            QThread::msleep(OMRON_DELAY_MS);
        }

        if (!Toolbox::Invoker::run(m_hardware, &Hardware::moveRackDown).result() ||
            !Toolbox::Invoker::runDelayed(rollbackTimeMs, m_hardware, &Hardware::moveRackStop).result()) {
            setLastError(tr("Не удалось откатить механику"));
            setState(State::Error);
            return false;
        }
    }

    if (!Toolbox::Invoker::run(m_hardware, &Hardware::unlockRemote).result()) {
        setLastError(tr("Не удалось разблокировать пульт"));
        setState(State::Error);
        return false;
    }

    setState(State::Idle);
    return true;
}

bool Scanner::makeCalibration()
{
    auto &settings = LocalSettings::instance();

    if (m_state != State::Idle) {
        setLastError(tr("Флюорограф не готов"));
        return false;
    }

    const quint8 doorsCount = settings.scannerDoorsCount();

    const auto scanningModes = ScanningModesCollection::instance().list();
    if (scanningModes.isEmpty()) {
        setLastError(tr("Список режимов сканирования пуст"));
        return false;
    }

    setState(State::Calibration);

    if (!Toolbox::Invoker::run(m_hardware, &Hardware::lockRemote).result()) {
        setLastError(tr("Не удалось заблокировать пульт"));
        setState(State::Error);
        return false;
    }

    if (doorsCount >= 1) {
        if (!Toolbox::Invoker::run(m_hardware, &Hardware::closeDoor, Hardware::DoorFirst).result()) {
            setLastError(tr("Не удалось закрыть дверь. %1").arg(m_hardware->lastError()));
            setState(State::Error);
            return false;
        }
    }

    int currentScanningMode = 0;

    for (auto &scanningMode : scanningModes) {
        m_currentAcquisitionResult = AcquisitionResult();

        if (!switchDevicesConfigurations(scanningMode)) {
            setState(State::Error);
            return false;
        }

        auto detectorProperties = m_detector->properties();

        m_currentAcquisitionResult.width = detectorProperties.width;
        m_currentAcquisitionResult.pixelSize = detectorProperties.pixelSizeMm;

        const auto linesCount = calculateDetectorLines(CALIBRATION_HEIGHT_MM, detectorProperties.pixelSizeMm.height());
        const auto darkLinesCount = calculateDetectorLines(DARK_FRAME_HEIGHT_MM, detectorProperties.pixelSizeMm.height());
        const auto exposureTimeMs = calculateExposureTime(linesCount, detectorProperties.chargeTimeMsec);

        {
            if (!pingDevices()) {
                setState(State::Error);
                return false;
            }

            auto hardwareOutcome = Toolbox::Invoker::run(m_hardware, &Hardware::moveRackToBottom);

            Toolbox::Invoker::Outcome<bool> powerSupplyOutcome;

            {
                PowerSupply::Params powerSupplyParams;
                powerSupplyParams.voltageKV = scanningMode.calibrationVoltageKv;
                powerSupplyParams.amperageMA = scanningMode.calibrationAmperageMa;
                powerSupplyParams.exposureMs = exposureTimeMs;

                powerSupplyOutcome = Toolbox::Invoker::run(m_powerSupply, &PowerSupply::prepare, powerSupplyParams);
            }

            Toolbox::Invoker::run(m_detector, &Detector::prepare, darkLinesCount).waitForFinished();
            auto detectorOutcome = Toolbox::Invoker::run(m_detector, &Detector::capture);

            if (detectorOutcome.result()) {
                detectorOutcome = Toolbox::Invoker::run(m_detector, &Detector::prepare, linesCount);
            }

            detectorOutcome.waitForFinished();
            hardwareOutcome.waitForFinished();
            powerSupplyOutcome.waitForFinished();

            if (!detectorOutcome.result()) {
                setLastError(tr("Не удалось подготовить детектор. %1").arg(m_detector->lastError()));
                setState(State::Error);
                return false;
            }

            if (!hardwareOutcome.result()) {
                setLastError(tr("Не удалось подготовить механику. %1").arg(m_hardware->lastError()));
                setState(State::Error);
                return false;
            }

            if (!powerSupplyOutcome.result()) {
                setLastError(tr("Не удалось подготовить РПУ. %1").arg(m_powerSupply->lastError()));
                setState(State::Error);
                return false;
            }

            if (!Toolbox::Invoker::run(m_hardware, &Hardware::pressPrepareButton).result()) {
                setLastError(tr("Не удалось нажать кнопку подготовки. %1").arg(m_hardware->lastError()));
                setState(State::Error);
                return false;
            }

            m_currentAcquisitionResult.dark = m_detector->lastCapturedFrame();
        }

        {
            bool fatalErrorOccurred = false;

            if (!Toolbox::Invoker::run(m_hardware, &Hardware::startScan, m_powerSupply).result()) {
                setLastError(tr("Не удалось начать движение механики. %1").arg(m_hardware->lastError()));
                setState(State::Error);
                return false;
            }

            auto detectorOutcome = Toolbox::Invoker::runDelayed(settings.scannerDetectorDelayMs(), m_detector, &Detector::capture);
            auto powerSupplyOutcome = Toolbox::Invoker::run(m_powerSupply, &PowerSupply::launch);

            if (!detectorOutcome.result()) {
                setLastError(tr("Не удалось получить изображение с детектора. %1").arg(m_detector->lastError()));
                fatalErrorOccurred = true;
            } else {
                m_currentAcquisitionResult.image = m_detector->lastCapturedFrame();
            }

            if (!Toolbox::Invoker::run(m_hardware, &Hardware::stopScan).result()) {
                setLastError(tr("Не удалось остановить механику. %1").arg(m_hardware->lastError()));
                fatalErrorOccurred = true;
            }

            if (!powerSupplyOutcome.result()) {
                setLastError(tr("Ошибка включения-выключения РПУ. %1").arg(m_powerSupply->lastError()));
                fatalErrorOccurred = true;
            }

            if (!Toolbox::Invoker::run(m_powerSupply, &PowerSupply::getResults).result()) {
                if (!fatalErrorOccurred) {
                    setLastError(tr("Не удалось получить результаты работы РПУ. %1").arg(m_powerSupply->lastError()));
                }

                accumulateReleasedPower(scanningMode.calibrationVoltageKv,
                                        scanningMode.calibrationAmperageMa,
                                        exposureTimeMs);
            } else {
                PowerSupply::Results powerResults = m_powerSupply->results();

                m_currentAcquisitionResult.amperageMa = powerResults.amperageMA;
                m_currentAcquisitionResult.voltageKv = powerResults.voltageKV;
                m_currentAcquisitionResult.exposureMs = powerResults.exposureMs;

                accumulateReleasedPower(m_currentAcquisitionResult.voltageKv,
                                        m_currentAcquisitionResult.amperageMa,
                                        m_currentAcquisitionResult.exposureMs);
            }

            if (fatalErrorOccurred) {
                setState(State::Error);
                return false;
            }

            {
                const int width = m_currentAcquisitionResult.width;
                const QVector<float> &image = m_currentAcquisitionResult.image;
                const QVector<float> &dark = m_currentAcquisitionResult.dark;

                bool updated = ScannerCalibrationData::instance().update(scanningMode.uuid(), image,
                                                                         dark, width);

                if (!updated) {
                    setLastError(tr("Не удалось обновить калибровочные коэффициенты"));
                    setState(State::Error);
                    return false;
                }
            }
        }

        QThread::msleep(OMRON_DELAY_MS);

        emit calibrationProgress(++currentScanningMode, scanningModes.size());
    }

    if (doorsCount >= 1) {
        if (!Toolbox::Invoker::run(m_hardware, &Hardware::openFirstDoor).result()) {
            setLastError(tr("Не удалось открыть дверь. %1").arg(m_hardware->lastError()));
            setState(State::Error);
            return false;
        }
    }

    if (!Toolbox::Invoker::run(m_hardware, &Hardware::unlockRemote).result()) {
        setLastError(tr("Не удалось разблокировать пульт. %1").arg(m_hardware->lastError()));
        setState(State::Error);
        return false;
    }

    setState(State::Idle);
    return true;
}

void Scanner::moveRackUp()
{
    if (!checkIsOpen()) {
        return;
    }

    Toolbox::Invoker::run(m_hardware, &Hardware::moveRackUp).waitForFinished();
}

void Scanner::moveRackDown()
{
    if (!checkIsOpen()) {
        return;
    }

    Toolbox::Invoker::run(m_hardware, &Hardware::moveRackDown).waitForFinished();
}

void Scanner::openFirstDoor(CancelationTokenSource::Token token)
{
    if (!checkIsOpen()) {
        return;
    }

    Toolbox::Invoker::run(m_hardware, &Hardware::openFirstDoor, token).waitForFinished();
}

void Scanner::closeFirstDoor(CancelationTokenSource::Token token)
{
    if (!checkIsOpen()) {
        return;
    }

    Toolbox::Invoker::run(m_hardware, &Hardware::closeFirstDoor, token).waitForFinished();
}

void Scanner::stop()
{
    if (!checkIsOpen()) {
        return;
    }

    Toolbox::Invoker::run(m_hardware, &Hardware::stop).waitForFinished();
}

bool Scanner::checkIsOpen()
{
    if (m_state == State::Unknown) {
        setLastError(tr("Необходимо выполнить инициализацию"));
        return false;
    }

    return true;
}

bool Scanner::reset()
{
    if (!checkIsOpen()) {
        return false;
    }

    if (!resetDevices()) {
        setState(State::Error);
        return false;
    }

    setState(State::Idle);
    return true;
}

Scanner::AcquisitionResult Scanner::lastAcquisitionResult() const
{
    return m_lastAcquisitionResult;
}

bool Scanner::isXRayOn() const
{
    return m_isXrayOn;
}

Scanner::State Scanner::state() const
{
    return m_state;
}

void Scanner::processAcqusitionResult(const ScanningModesCollection::Item &scanningMode)
{
    const int width = m_currentAcquisitionResult.width;
    QVector<float> &imageFrame = m_currentAcquisitionResult.image;
    const QVector<float> &darkFrame = m_currentAcquisitionResult.dark;

    if (imageFrame.isEmpty() || imageFrame.size() % width) {
        return;
    }

    if (LocalSettings::instance().scannerFlatFieldCorrectionEnabled()) {
        if (!ScannerCalibrationData::instance().apply(scanningMode.uuid(), imageFrame, darkFrame, width)) {
            setLastError(tr("Возникла ошибка при выполнении нормировки"));
        }
    }

    m_lastAcquisitionResult = m_currentAcquisitionResult;
    emit acquisitionResultReady(m_lastAcquisitionResult);
}

bool Scanner::openDevices()
{
    const auto availableScanningModes = ScanningModesCollection::instance().list();
    if (availableScanningModes.isEmpty()) {
        setLastError(tr("Нет доступных режимов сканирования"));
        return false;
    }

    const auto &defaultScanningMode = availableScanningModes.first();

    if (!Toolbox::Invoker::run(m_dispatcher, &Dispatcher::isOpen).result()) {
        auto &settings = LocalSettings::instance();

        Dispatcher::Params params;
        params.portName = settings.dispatcherSerialPortName();
        params.writeDelayMs = settings.dispatcherWriteDelayMs();
        params.writeTimeoutMs = settings.dispatcherWriteTimeoutMs();
        params.writePauseMs = settings.dispatcherWritePauseMs();
        params.frameIntervalMs = settings.dispatcherFrameIntervalMs();

        if (!Toolbox::Invoker::run(m_dispatcher, &Dispatcher::open, params).result()) {
            setLastError(tr("Ошибка диспетчера. %1").arg(m_dispatcher->lastError()));
            return false;
        }
    }

    if (!Toolbox::Invoker::run(m_dispatcher, &Dispatcher::reset).result()) {
        setLastError(tr("Ошибка диспетчера. %1").arg(m_dispatcher->lastError()));
        return false;
    }

    QMap<Device *, Toolbox::Invoker::Outcome<bool>> outcomes;

    for (auto &device : devices()) {
        if (device && !Toolbox::Invoker::run(device, &Device::isOpen).result()) {
            auto config = defaultScanningMode.devicesConfigurations.value(device->name());
            outcomes.insert(device, Toolbox::Invoker::run(device, &Device::open, config));
        }
    }

    bool success = true;

    for (auto it = outcomes.cbegin(); it != outcomes.cend(); ++it) {
        if (!it.value().result() && success) {
            setLastError(deviceLastError(it.key()));
            success = false;
        }
    }

    return success;
}

bool Scanner::resetDevices()
{
    QMap<Device *, Toolbox::Invoker::Outcome<bool>> outcomes;

    for (auto &device : devices()) {
        if (device && Toolbox::Invoker::run(device, &Device::isOpen).result()) {
            outcomes.insert(device, Toolbox::Invoker::run(device, &Device::reset));
        }
    }

    bool success = true;

    for (auto it = outcomes.cbegin(); it != outcomes.cend(); ++it) {
        if (!it.value().result() && success) {
            setLastError(deviceLastError(it.key()));
            success = false;
        }
    }

    if (m_dispatcher) {
        if (!Toolbox::Invoker::run(m_dispatcher, &Dispatcher::reset).waitForFinished() && success) {
            setLastError(m_dispatcher->lastError());
            success = false;
        }
    }

    return success;
}

void Scanner::closeDevices()
{
    QMap<Device *, Toolbox::Invoker::Outcome<void>> outcomes;

    for (auto &device : devices()) {
        if (device) {
            outcomes.insert(device, Toolbox::Invoker::run(device, &Device::close));
        }
    }

    for (auto &outcome : outcomes) {
        outcome.waitForFinished();
    }

    if (m_dispatcher) {
        Toolbox::Invoker::run(m_dispatcher, &Dispatcher::close).waitForFinished();
    }
}

bool Scanner::switchDevicesConfigurations(const ScanningModesCollection::Item &scanningMode)
{
    if (!scanningMode.isEnabled) {
        setLastError(tr("Режим сканирования %1 выключен").arg(scanningMode.uuid()));
        return false;
    }

    QMap<Device *, Toolbox::Invoker::Outcome<bool>> outcomes;

    for (auto &device : devices()) {
        auto config = scanningMode.devicesConfigurations.value(device->name());
        outcomes.insert(device, Toolbox::Invoker::run(device, &Device::reOpen, config));
    }

    bool success = true;

    for (auto it = outcomes.cbegin(); it != outcomes.cend(); ++it) {
        if (!it.value().result() && success) {
            setLastError(deviceLastError(it.key()));
            success = false;
        }
    }

    return success;
}

void Scanner::accumulateReleasedPower(double kV, double mA, ushort exposureMs)
{
    auto &s = LocalSettings::instance();

    const QDateTime now = QDateTime::currentDateTime();

    if (!m_cooldownDateTime.isValid() || m_cooldownDateTime < now) {
        m_cooldownDateTime = now;
    }

    int seconds = qCeil(kV * mA * exposureMs / 1000 / s.scannerCoolingRateWattSec());
    m_cooldownDateTime = m_cooldownDateTime.addSecs(seconds);

    m_run->setValue(kCooldownTimeParam, m_cooldownDateTime);
}

QVector<Device *> Scanner::devices() const
{
    QVector<Device *> devices = {
        m_detector,
        m_powerSupply,
        m_hardware
    };

    return devices;
}

QString Scanner::deviceLastError(Device *device)
{
    Q_ASSERT(device);

    QString deviceType;
    if (qobject_cast<Detector *>(device)) {
        deviceType = tr("Ошибка детектора");
    } else if (qobject_cast<Hardware *>(device)) {
        deviceType = tr("Ошибка механики");
    } else if (qobject_cast<PowerSupply *>(device)) {
        deviceType = tr("Ошибка РПУ");
    } else {
        deviceType = tr("Ошибка неизвестного устройства");
    }

    return QStringLiteral("%1. %2").arg(deviceType).arg(device->lastError());
}

bool Scanner::configureDispatcher()
{
    if (m_dispatcher) {
        warnDevice << "Dispatcher already configured";
        return true;
    }

    m_dispatcherThread.start(QThread::HighestPriority);

    m_dispatcher = new Dispatcher;
    m_dispatcher->moveToThread(&m_dispatcherThread);
    connect(&m_dispatcherThread, &QThread::finished, m_dispatcher, &QObject::deleteLater);

    return true;
}

QString Scanner::lastError() const
{
    return m_lastError;
}

quint32 Scanner::secsToCooldown() const
{
    const auto now = QDateTime::currentDateTime();

    if (!m_cooldownDateTime.isValid() || m_cooldownDateTime < now) {
        return 0;
    }

    return qMax(static_cast<qint64>(0), 
                now.secsTo(m_cooldownDateTime) - LocalSettings::instance().scannerCoolingThresholdSec());
}

QDataStream &operator<<(QDataStream &stream, const Scanner::AcquisitionResult &result)
{
    stream << result.amperageMa;
    stream << result.voltageKv;
    stream << result.exposureMs;
    stream << result.width;
    stream << result.image;
    stream << result.pixelSize;

    return stream;
}

QDataStream &operator>>(QDataStream &stream, Scanner::AcquisitionResult &result)
{
    stream >> result.amperageMa;
    stream >> result.voltageKv;
    stream >> result.exposureMs;
    stream >> result.width;
    stream >> result.image;
    stream >> result.pixelSize;

    return stream;
}
