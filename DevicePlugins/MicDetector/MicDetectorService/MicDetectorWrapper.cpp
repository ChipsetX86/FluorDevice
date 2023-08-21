#include "MicDetectorWrapper.h"

#include <QLibrary>
#include <QDebug>

#include "mic.h"

namespace {
    typedef WORD (*HardwareInit_)(char*, WORD);
    typedef void (*HardwareShutdown_)();
    typedef bool (*PrepareReading_)(float, WORD, WORD);
    typedef void (*ReadHardwareInfo_) (HARDWARE_INFO*);
    typedef WORD (*ReadData_)(WORD*, WORD, WORD);
    typedef bool (*CloseReading_)(WORD*, WORD, WORD, bool);

    const QString NAME_DLL_MIC = QStringLiteral("hardware.dll");
}

struct MicDetectorWrapper::PImpl
{
    bool isInitialized = false;
    bool isLibLoaded = false;
    QLibrary sdkLibrary;
    ushort widthPx = 0;
    ushort prepareLinesCount = 0;
    QString lastError;

    HardwareInit_ fHardwareInit;
    HardwareShutdown_ fHardwareShutdown;
    PrepareReading_ fPrepareReading;
    ReadHardwareInfo_ fReadHardwareInfo;
    ReadData_ fReadData;
    CloseReading_ fCloseReading;

    template <typename T>
    bool resolveFunction(T &ptr, const char *symbol)
    {
        if (!(ptr = reinterpret_cast<T>(sdkLibrary.resolve(symbol)))) {
            qCritical() << "Failed to resolve symbol:" << symbol;
            return false;
        }

        return true;
    }

    bool checkInitDetector();
};

MicDetectorWrapper::MicDetectorWrapper(QObject *parent): QObject(parent),
    m_pimpl(new PImpl)
{

}

bool MicDetectorWrapper::loadMicLibrary()
{
    if (m_pimpl->isLibLoaded) {
        return true;
    }

    m_pimpl->sdkLibrary.setFileName(NAME_DLL_MIC);

    bool resolved = m_pimpl->resolveFunction(m_pimpl->fHardwareInit, "_HardwareInit") &&
                    m_pimpl->resolveFunction(m_pimpl->fHardwareShutdown, "_HardwareShutdown") &&
                    m_pimpl->resolveFunction(m_pimpl->fPrepareReading, "_PrepareReading") &&
                    m_pimpl->resolveFunction(m_pimpl->fReadHardwareInfo, "_ReadHardwareInfo") &&
                    m_pimpl->resolveFunction(m_pimpl->fReadData, "_ReadData") &&
                    m_pimpl->resolveFunction(m_pimpl->fCloseReading, "_CloseReading");

    if (!resolved) {
        m_pimpl->lastError = tr("Ошибка поиска функций в библиотеке");
        return false;
    }

    m_pimpl->isLibLoaded = true;
    return true;
}

bool MicDetectorWrapper::hardwareInit()
{
    if (!m_pimpl->isLibLoaded) {
        m_pimpl->lastError = tr("Библиотека не загружена");
        return false;
    }

    if (m_pimpl->isInitialized) {
        return true;
    }

    if (ushort code = m_pimpl->fHardwareInit(0, 0)) {
        m_pimpl->isInitialized = false;
        m_pimpl->lastError = tr("Ошибка инициализации детектора. Код ошибки %1").
                arg(QString::number(code, 16));
    } else {
        m_pimpl->isInitialized = true;
        HardwareInfo info;
        if (readHardwareInfo(info)) {
            m_pimpl->widthPx = info.widthPx;
        } else {
            m_pimpl->lastError = tr("Не удалось получить информацию о детекторе");
            return false;
        }
    }
    return true;
}

bool MicDetectorWrapper::readHardwareInfo(HardwareInfo& info)
{
    if (!m_pimpl->checkInitDetector()) {
        return false;
    }

    HARDWARE_INFO hardwareInfo;
    m_pimpl->fReadHardwareInfo(&hardwareInfo);
    if (hardwareInfo.MaxXdim) {
        info.widthPx = hardwareInfo.MaxXdim;
        qDebug() << "MaxXdim" << hardwareInfo.MaxXdim
                  << "NumModules" << hardwareInfo.NumModules
                  << "NumChannels" << hardwareInfo.NumChannels;
    } else {
        m_pimpl->lastError = tr("Получены неверные данные о детекторе");
        return false;
    }

    return true;
}

bool MicDetectorWrapper::hardwareShutdown()
{
    if (!m_pimpl->checkInitDetector()) {
        return false;
    }

    m_pimpl->fHardwareShutdown();
    m_pimpl->isInitialized = false;

    return true;
}

bool MicDetectorWrapper::prepareReading(const ushort linesCount, const float chargeTimeMs)
{
    if (!m_pimpl->checkInitDetector()) {
        return false;
    }

    if (m_pimpl->fPrepareReading(chargeTimeMs, m_pimpl->widthPx, linesCount)) {
        m_pimpl->prepareLinesCount = linesCount;
    } else {
        m_pimpl->lastError = tr("Подготовка детектора к снимку завершилась с ошибкой");
        return false;
    }

    return true;
}

bool MicDetectorWrapper::readData(MicFrame &frame)
{
    if (!m_pimpl->checkInitDetector()) {
        return false;
    }

    if (!m_pimpl->prepareLinesCount) {
        m_pimpl->lastError = tr("Высота снимка равна 0 строк");
        return false;
    }

    MicFrame buffFrame(m_pimpl->widthPx * m_pimpl->prepareLinesCount);
    ushort realReadLines = m_pimpl->fReadData(buffFrame.data(), m_pimpl->widthPx, m_pimpl->prepareLinesCount);
    bool isErrorClose = m_pimpl->fCloseReading(buffFrame.data(), m_pimpl->widthPx, realReadLines, true);
    if (!isErrorClose || m_pimpl->prepareLinesCount < realReadLines || !realReadLines) {
        m_pimpl->prepareLinesCount = 0;
        m_pimpl->lastError = tr("Ошибка получения данных");
        return false;
    }

    if (m_pimpl->prepareLinesCount == realReadLines) {
        frame = buffFrame;
    } else {
        frame = buffFrame.mid(0, realReadLines * m_pimpl->widthPx);
    }

    m_pimpl->prepareLinesCount = 0;
    return true;
}

QString MicDetectorWrapper::lastError()
{
    return m_pimpl->lastError;
}

bool MicDetectorWrapper::PImpl::checkInitDetector()
{
    if (!isInitialized) {
        lastError = tr("Инициализация детектора не завершена");
        return false;
    }
    return true;
}
