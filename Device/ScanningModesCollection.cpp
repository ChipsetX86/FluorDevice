#include "ScanningModesCollection.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QSettings>
#include <QUuid>
#include <algorithm>

#include <Settings/LocalSettings.h>

#include "DeviceLogging.h"

namespace {
    bool itemIsLess(const ScanningModesCollection::Item &a, const ScanningModesCollection::Item &b)
    {
        if (a.sortOrder == b.sortOrder) {
            if (a.title == b.title) {
                return a.uuid() < b.uuid();
            }

            return a.title < b.title;
        }

        return a.sortOrder < b.sortOrder;
    }

    const QUuid::StringFormat kUUIDFormat = QUuid::WithoutBraces;
    const QString kScanningModeConfigFilename = QStringLiteral("ScanningMode.cfg");
    const QString kTitleParam = QStringLiteral("main/title");
    const QString kEnabledParam = QStringLiteral("main/enabled");
    const QString kSortOrderParam = QStringLiteral("main/sort_order");
    const QString kCalibrationAmperageMaParam = QStringLiteral("main/calibration_amperage_ma");
    const QString kCalibrationVoltageKvParam = QStringLiteral("main/calibration_voltage_kv");
    const QString kMinAmperageMaParam = QStringLiteral("main/min_amperage_ma");
    const QString kMaxAmperageMaParam = QStringLiteral("main/max_amperage_ma");
    const QString kMinVoltageKvParam = QStringLiteral("main/min_voltage_kv");
    const QString kMaxVoltageKvParam = QStringLiteral("main/max_voltage_kv");
    const QString kSnapshotWidthParam = QStringLiteral("main/snapshot_width_px");
    const QString kRollbackAlphaParam = QStringLiteral("main/rollback_alpha");
    const QString kRollbackBetaParam = QStringLiteral("main/rollback_beta");
    const QString kBinningSumParam = QStringLiteral("binning/sum");
    const QString kBinningHorizontalParam = QStringLiteral("binning/horizontal");
    const QString kBinningVerticalParam = QStringLiteral("binning/vertical");
}

static const int qmtScanningModesCollectionItem = qRegisterMetaType<ScanningModesCollection::Item>();

ScanningModesCollection &ScanningModesCollection::instance()
{
    static ScanningModesCollection instance(LocalSettings::instance().dirPath() +
                                            QStringLiteral("/ScanningModes"));
    return instance;
}

ScanningModesCollection::Item ScanningModesCollection::create()
{    
    Item i;

    do {
        i.m_uuid = QUuid::createUuid().toString(kUUIDFormat);
    } while (m_items.contains(i.m_uuid));

    return i;
}

ScanningModesCollection::ItemsList ScanningModesCollection::list(bool onlyEnabled, bool sorted) const
{
    ItemsList result;
    for (auto &item : qAsConst(m_items)) {
        if (onlyEnabled && !item.isEnabled) {
            continue;
        }

        result.append(item);
    }

    if (sorted) {
        std::sort(result.begin(), result.end(), itemIsLess);
    }

    return result;
}

const ScanningModesCollection::ItemsMap &ScanningModesCollection::items() const
{
    return m_items;
}

bool ScanningModesCollection::store(const ScanningModesCollection::ItemsList &list)
{
    QDir scanningModesDirectory(m_path);

    infoDevice << "Removing exists scanning modes from" << scanningModesDirectory.absolutePath();

    QDirIterator it(scanningModesDirectory.absolutePath(), QDir::AllDirs | QDir::NoDotAndDotDot,
                    QDirIterator::NoIteratorFlags);
    while (it.hasNext()) {
        const QString scanningModePath = it.next();
        QDir scanningModeDir(scanningModePath);
        const QUuid uuid(scanningModeDir.dirName());
        if (uuid.isNull()) {
            warnDevice << "Not is scanning mode:" << scanningModePath;
            continue;
        }

        if (!scanningModeDir.removeRecursively()) {
            errDevice << "Failed to remove scanning mode:" << scanningModePath;
            return false;
        }
    }

    infoDevice << "Storing scanning modes into" << scanningModesDirectory.absolutePath();

    for (auto &item : list) {
        const QUuid uuid(item.uuid());
        if (uuid.isNull()) {
            errDevice << "Scanning mode UUID is null";
            return false;
        }

        const QString scanningModePath = scanningModesDirectory.absolutePath() +
                                         QStringLiteral("/%1").arg(item.uuid());

        if (!QDir().mkpath(scanningModePath)) {
            errDevice << "Failed create scanning mode directory:" << scanningModePath;
            return false;
        }

        const QString scanningModeConfigFilename = scanningModePath +
                                                   QStringLiteral("/%1").arg(kScanningModeConfigFilename);

        {
            QSettings config(scanningModeConfigFilename, QSettings::IniFormat);
            config.setValue(kTitleParam, item.title);
            config.setValue(kEnabledParam, item.isEnabled);
            config.setValue(kSortOrderParam, item.sortOrder);
            config.setValue(kCalibrationAmperageMaParam, item.calibrationAmperageMa);
            config.setValue(kCalibrationVoltageKvParam, item.calibrationVoltageKv);
            config.setValue(kMinAmperageMaParam, item.minAmperageMa);
            config.setValue(kMaxAmperageMaParam, item.maxAmperageMa);
            config.setValue(kMinVoltageKvParam, item.minVoltageKv);
            config.setValue(kMaxVoltageKvParam, item.maxVoltageKv);
            config.setValue(kBinningSumParam, item.binningSum);
            config.setValue(kBinningHorizontalParam, item.binningHorizontal);
            config.setValue(kBinningVerticalParam, item.binningVertical);
            config.setValue(kSnapshotWidthParam, item.snapshotWidth);
            config.setValue(kRollbackAlphaParam, item.rollbackAlpha);
            config.setValue(kRollbackBetaParam, item.rollbackBeta);

            config.sync();
            if (config.status() != QSettings::Status::NoError) {
                errDevice << "Failed to save config file:" << scanningModeConfigFilename;
                return false;
            }
        }

        for (auto it = item.devicesConfigurations.cbegin(); it != item.devicesConfigurations.cend(); ++it) {
            const QString deviceName = it.key();
            if (deviceName.isEmpty()) {
                errDevice << "Bad device name";
                return false;
            }

            const QString deviceConfigurationFilename = scanningModePath +
                                                        QStringLiteral("/%1.ini").arg(deviceName);

            if (!it->save(deviceConfigurationFilename)) {
                errDevice << "Failed to save device configuration:" << deviceConfigurationFilename;
                return false;
            }
        }

        infoDevice << "Scanning mode" << item.uuid() << "successfully stored";
    }

    infoDevice << "All scanning modes are successfully stored";

    m_items.clear();
    for (auto &item : list) {
        m_items.insert(item.uuid(), item);
    }

    return true;
}

ScanningModesCollection::ScanningModesCollection(const QString &path)
{   
    QDir dir(path);
    dir.mkpath(path);
    m_path = dir.absolutePath();
    load();
}

void ScanningModesCollection::load()
{
    QDir scanningModesDirectory(m_path);

    infoDevice << "Searching scanning modes in" << scanningModesDirectory.absolutePath();

    QDirIterator it(scanningModesDirectory.absolutePath(), {kScanningModeConfigFilename},
                    QDir::Files | QDir::CaseSensitive,
                    QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString scanningModeConfigFilename = it.next();
        const QDir scanningModeDir = QFileInfo(scanningModeConfigFilename).absoluteDir();
        const QUuid uuid(scanningModeDir.dirName());
        if (uuid.isNull()) {
            warnDevice << "Scanning mode UUID is null:" << scanningModeConfigFilename;
            continue;
        }

        {
            auto parentScanningModeDir = scanningModeDir;
            if (!parentScanningModeDir.cdUp() ||
                parentScanningModeDir.absolutePath() != scanningModesDirectory.absolutePath()) {
                warnDevice << "Scanning mode bad directory structure:" << scanningModeConfigFilename;
                continue;
            }
        }

        ScanningModesCollection::Item scanningMode;
        scanningMode.m_uuid = uuid.toString(kUUIDFormat);

        {
            QSettings config(scanningModeConfigFilename, QSettings::IniFormat);
            scanningMode.title = config.value(kTitleParam).toString();
            scanningMode.isEnabled = config.value(kEnabledParam, scanningMode.isEnabled).toBool();
            scanningMode.sortOrder = config.value(kSortOrderParam, scanningMode.sortOrder).value<quint16>();
            scanningMode.calibrationAmperageMa = config.value(kCalibrationAmperageMaParam, scanningMode.calibrationAmperageMa).toReal();
            scanningMode.calibrationVoltageKv = config.value(kCalibrationVoltageKvParam, scanningMode.calibrationVoltageKv).toReal();
            scanningMode.minAmperageMa = config.value(kMinAmperageMaParam, scanningMode.minAmperageMa).toReal();
            scanningMode.maxAmperageMa = config.value(kMaxAmperageMaParam, scanningMode.maxAmperageMa).toReal();
            scanningMode.minVoltageKv = config.value(kMinVoltageKvParam, scanningMode.minVoltageKv).toReal();
            scanningMode.maxVoltageKv = config.value(kMaxVoltageKvParam, scanningMode.maxVoltageKv).toReal();
            scanningMode.binningSum = config.value(kBinningSumParam, scanningMode.binningSum).toBool();
            scanningMode.binningHorizontal = config.value(kBinningHorizontalParam, scanningMode.binningHorizontal).value<quint16>();
            scanningMode.binningVertical = config.value(kBinningVerticalParam, scanningMode.binningVertical).value<quint16>();
            scanningMode.snapshotWidth = config.value(kSnapshotWidthParam, scanningMode.snapshotWidth).value<quint16>();
            scanningMode.rollbackAlpha = config.value(kRollbackAlphaParam, scanningMode.rollbackAlpha).value<quint16>();
            scanningMode.rollbackBeta = config.value(kRollbackBetaParam, scanningMode.rollbackBeta).toReal();
        }

        QDirIterator deviceIt(scanningModeDir.absolutePath(), {QStringLiteral("*.ini")},
                               QDir::Files, QDirIterator::NoIteratorFlags);
        while (deviceIt.hasNext()) {
            const QString deviceFilename = deviceIt.next();
            const QString deviceName = QFileInfo(deviceFilename).completeBaseName();
            if (deviceName.isEmpty()) {
                warnDevice << "Device configuration plugin name is empty";
                continue;
            }

            DeviceConfiguration deviceConfiguration(deviceFilename);
            scanningMode.devicesConfigurations.insert(deviceName, deviceConfiguration);
        }

        infoDevice << "Scanning mode" << scanningMode.uuid() << "successfully loaded";
        m_items.insert(scanningMode.uuid(), scanningMode);
    }

    infoDevice << "Total available scanning modes:" << m_items.size();
}

ScanningModesCollection::Item::Item() :
    isEnabled(false),
    sortOrder(0),
    calibrationAmperageMa(0),
    calibrationVoltageKv(0),
    minAmperageMa(0),
    maxAmperageMa(0),
    minVoltageKv(0),
    maxVoltageKv(0),
    binningSum(false),
    binningHorizontal(0),
    binningVertical(0),
    snapshotWidth(0),
    rollbackAlpha(0),
    rollbackBeta(1)
{

}

bool ScanningModesCollection::Item::isNull() const
{
    return m_uuid.isNull();
}

void ScanningModesCollection::Item::copy(const ScanningModesCollection::Item &other)
{
    QString oldUuid = m_uuid;
    *this = other;
    m_uuid = oldUuid;
}

bool ScanningModesCollection::Item::operator==(const ScanningModesCollection::Item &other) const
{
    return m_uuid == other.m_uuid &&
           title == other.title &&
           isEnabled == other.isEnabled &&
           sortOrder == other.sortOrder &&
           qFuzzyCompare(calibrationAmperageMa, other.calibrationAmperageMa) &&
           qFuzzyCompare(calibrationVoltageKv, other.calibrationVoltageKv) &&
           qFuzzyCompare(minAmperageMa, other.minAmperageMa) &&
           qFuzzyCompare(maxAmperageMa, other.maxAmperageMa) &&
           qFuzzyCompare(minVoltageKv, other.minVoltageKv) &&
           qFuzzyCompare(maxVoltageKv, other.maxVoltageKv) &&
           binningSum == other.binningSum &&
           binningHorizontal == other.binningHorizontal &&
           binningVertical == other.binningVertical &&
           snapshotWidth == other.snapshotWidth &&
           rollbackAlpha == other.rollbackAlpha &&
           qFuzzyCompare(rollbackBeta, rollbackBeta) &&
           devicesConfigurations == other.devicesConfigurations;
}
