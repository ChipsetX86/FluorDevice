#include "SslDetector.h"

#include <QApplication>
#include <QDir>
#include <QStandardPaths>
#include <QLibrary>
#include <QThread>

#include <Device/DeviceLogging.h>

#if defined(Q_OS_WIN32)
const QString LIBRARY_FILENAME = QStringLiteral("p_dll_udp.dll");
#elif defined(Q_OS_LINUX)
const QString LIBRARY_FILENAME = QStringLiteral("libp_dll_udp.so");
#else
#error Support your platform here
#endif
const QString GANE_MTR = QStringLiteral("main/gane_mtr");
const QString TEST_MODE = QStringLiteral("main/test_mode");
const QString CHARGE_TIME = QStringLiteral("main/charge_time_ms");
const QString PIXEL_WIDTH = QStringLiteral("main/pixel_width");
const QString PIXEL_HEIGHT = QStringLiteral("main/pixel_height");
const QString MATRIX_COUNT = QStringLiteral("main/matrix_count");
const QString PIXELS_PER_MATRIX = QStringLiteral("main/pixels_per_matrix");
const QString JUNK_LINES = QStringLiteral("main/junk_lines");
const QString FIX_MATRIX_JOINT = QStringLiteral("main/fix_matrix_joint");
const int SIZE_JOINT_PIXELS = 2;

struct SslDetector::PImpl
{
public:
    enum Mode {
        RUN,
        TEST
    };

    typedef char *(*GetVersionFunc)();
    typedef unsigned long (*GetBuferFunc)(char **, ulong, bool);
    typedef void (*InitFunc)(const char* , const char *);
    typedef bool (*AfteInitOnCreateFunc)();
    typedef void (*Dll_CloseFunc)();
    typedef bool (*SendIcmpPackageFunc)();
    typedef ushort (*GetPixelPerStringFunc)();
    typedef ushort (*GetChErrorFunc)();
    typedef void (*SetBinningFunc)(ushort);
    typedef ushort (*GetBinningFunc)();
    typedef void (*SetModeRunFunc)(Mode);
    typedef ulong (*GetErrorFunc)(char **);
    typedef void (*PutGaneMtrFunc)(uchar);
    typedef uchar (*GetGaneMtrFunc)();

    GetVersionFunc getVersion;
    AfteInitOnCreateFunc aioc;
    SendIcmpPackageFunc sendIcmp;
    Dll_CloseFunc close;
    GetBuferFunc getBuff;
    InitFunc init;
    GetPixelPerStringFunc getPixelPerString;
    GetChErrorFunc getChError;
    SetBinningFunc setBinning;
    GetBinningFunc getBinning;
    SetModeRunFunc setModeRun;
    GetErrorFunc getError;
    PutGaneMtrFunc putGaneMtr;
    GetGaneMtrFunc getGaneMtr;

    ushort hardwareWidth;
    int matrixCount;
    int pixelsPerMatrix;
    bool fixesMatrixJoint;

    bool loadLibrary();
    void unloadLibrary();

    void convertRawToVector(const char *raw, QVector<float>& vector,
                            const ushort linesCount) const;
    ulong calcBufferSize(uint lines) const;
    int calcResultFrameWidth() const;
    void fixMatrixJoint(Frame &frame, int linesCount) const;

    template <typename T>
    bool resolveFunction(T &ptr, const char *symbol)
    {
        if (!(ptr = reinterpret_cast<T>(library.resolve(symbol)))) {
            errDevice << "Failed to resolve symbol:" << symbol;
            return false;
        }

        return true;
    }
private:
    QLibrary library;
};

SslDetector::SslDetector(QObject *parent) :
    Detector(parent),
    m_pimpl(new PImpl)
{

}

ulong SslDetector::PImpl::calcBufferSize(uint lines) const
{
    return lines * hardwareWidth * 2;
}

int SslDetector::PImpl::calcResultFrameWidth() const
{
    return hardwareWidth + (fixesMatrixJoint ? SIZE_JOINT_PIXELS * (matrixCount - 1) : 0);
}

void SslDetector::PImpl::fixMatrixJoint(Frame &frame, int linesCount) const
{
    if (!fixesMatrixJoint) {
        return;
    }

    int fixFrameWidth = calcResultFrameWidth();
    Frame fixFrame(fixFrameWidth * linesCount, 0);
    for (int i = 0; i < hardwareWidth; ++i) {
        for (int j = 0; j < linesCount; ++j) {
            int pos = j * fixFrameWidth + i + (i / pixelsPerMatrix) * SIZE_JOINT_PIXELS;
            fixFrame[pos] = frame[j * hardwareWidth + i];
        }
    }

    for (int i = pixelsPerMatrix; i < fixFrameWidth - 1; i += pixelsPerMatrix + SIZE_JOINT_PIXELS) {

        fixFrame[i] = fixFrame[i - 1];
        fixFrame[i + 1] = fixFrame[i + 2];
        int lastPixel = (linesCount - 1) * fixFrameWidth + i;
        fixFrame[lastPixel] = fixFrame[lastPixel - 1];
        fixFrame[lastPixel + 1] = fixFrame[lastPixel + 2];

        for (int j = 1; j < linesCount - 1; ++j) {
            int pos = j * fixFrameWidth + i;
            float count  = 3;
            fixFrame[pos] = (fixFrame[pos - 1] +
                             fixFrame[pos - fixFrameWidth - 1] +
                             fixFrame[pos + fixFrameWidth - 1]) / count;
            pos++;
            fixFrame[pos] = (fixFrame[pos + 1] +
                             fixFrame[pos - fixFrameWidth + 1] +
                             fixFrame[pos + fixFrameWidth + 1]) / count;
        }
    }

    frame = fixFrame;
}

SslDetector::~SslDetector()
{

}

bool SslDetector::PImpl::loadLibrary()
{
    library.setFileName(QApplication::applicationDirPath() + QStringLiteral("/") + LIBRARY_FILENAME);
    if (!library.load()) {
        errDevice << "Failed to load library:" << library.errorString();
        return false;
    }

    return resolveFunction(getVersion, "p_dll_GetVersionDll") &&
           resolveFunction(init, "Init") &&
           resolveFunction(aioc, "AfteInitOnCreate") &&
           resolveFunction(close, "p_dll_Close") &&
           resolveFunction(getBuff, "GetBufer") &&
           resolveFunction(getPixelPerString, "p_dll_GetPixelPerString") &&
           resolveFunction(sendIcmp, "p_dll_SendIcmp") &&
           resolveFunction(getChError, "p_dll_GetChError") &&
           resolveFunction(setBinning, "p_dll_SendBinning") &&
           resolveFunction(getBinning, "p_dll_GetBinning") &&
           resolveFunction(setModeRun, "p_dll_SetModeRun") &&
           resolveFunction(getError, "p_dll_GetError") &&
           resolveFunction(putGaneMtr, "p_dll_PutGaneMtr") &&
           resolveFunction(getGaneMtr, "p_dll_GetMtrGane");
}

void SslDetector::PImpl::unloadLibrary()
{
    library.unload();
}

void SslDetector::PImpl::convertRawToVector(const char *raw, Frame &vector,
                                            const ushort linesCount) const
{
    auto res16b = reinterpret_cast<const quint16 *>(raw);

    vector.clear();
    vector.reserve(linesCount * hardwareWidth);

    for (int i = linesCount - 1; i >= 0; --i) {
        for (int k = 0; k < matrixCount; ++k) {
            for (int j = 0; j < pixelsPerMatrix; ++j) {
                vector.append(*(res16b + i * hardwareWidth + matrixCount * (pixelsPerMatrix - 1 - j) + k));
            }
        }
    }

    //specifics of detector without checksums (last pixel broken)
    if (vector.size() >= hardwareWidth) {
        vector[vector.size() - pixelsPerMatrix] =
                vector[vector.size() - pixelsPerMatrix - 1];
    }
}

bool SslDetector::doOpen()
{
    auto &cfg = currentConfiguration();

    m_pimpl->matrixCount = cfg.value(MATRIX_COUNT).toInt();
    if (m_pimpl->matrixCount < 1) {
        setLastError(tr("Неподдерживаемое кол-во матриц"));
        return false;
    }

    m_pimpl->pixelsPerMatrix = cfg.value(PIXELS_PER_MATRIX).toInt();
    if (m_pimpl->pixelsPerMatrix < 1) {
        setLastError(tr("Неподдерживаемое кол-во пикселей в матрице"));
        return false;
    }

    if (!m_pimpl->loadLibrary()) {
        setLastError(tr("Не удалось загрузить библиотеку: %1").arg(LIBRARY_FILENAME));
        return false;
    }
    
    #if defined(Q_OS_WIN32)
    const QString cfgPath = QStringLiteral("C:/Users/Public");
    #elif defined(Q_OS_LINUX)
    const QString cfgPath = QCoreApplication::applicationDirPath();
    #else
    #error Support your platform here
    #endif

    infoDevice << "Using path:" << cfgPath;
    m_pimpl->init(nullptr, QDir::toNativeSeparators(cfgPath).toStdString().data());

    if (m_pimpl->aioc()) {
        setLastError(tr("Возникла ошибка при инициализации детектора ТЛД"));
        m_pimpl->close();
        m_pimpl->unloadLibrary();
        return false;
    }

    m_pimpl->putGaneMtr(cfg.value(GANE_MTR).toInt());
    
    QThread::msleep(2000);
    m_pimpl->setModeRun(cfg.value(TEST_MODE).toBool() ? PImpl::TEST : PImpl::RUN);
    m_pimpl->hardwareWidth = m_pimpl->getPixelPerString();

    m_pimpl->fixesMatrixJoint = cfg.value(FIX_MATRIX_JOINT).toBool();

    Properties props;
    props.chargeTimeMsec = cfg.value(CHARGE_TIME).toReal();
    props.width = m_pimpl->calcResultFrameWidth();
    props.pixelSizeMm.setWidth(cfg.value(PIXEL_WIDTH).toDouble());
    props.pixelSizeMm.setHeight(cfg.value(PIXEL_HEIGHT).toDouble());
    setProperties(props);

    return true;
}

void SslDetector::doClose()
{
    m_pimpl->close();
    m_pimpl->unloadLibrary();
}

bool SslDetector::doTestConnection()
{
    return m_pimpl->sendIcmp();
}

bool SslDetector::doPrepare()
{
    if (auto junkLines = currentConfiguration().value(JUNK_LINES).toUInt()) {
        char *buffer = nullptr;
        ulong requestedBufferSize = m_pimpl->calcBufferSize(junkLines);
        ulong actualBufferSize = m_pimpl->getBuff(&buffer, requestedBufferSize, true);
        if (actualBufferSize != requestedBufferSize) {
            setLastError(tr("Полученое кол-во байт (%1) меньше запрошенного (%2)").arg(actualBufferSize)
                                                                                  .arg(requestedBufferSize));
            return false;
        }
    }

    return true;
}

bool SslDetector::doCapture()
{
    auto linesCount = currentLines();

    char *buffer = nullptr;
    ulong requestedBufferSize = m_pimpl->calcBufferSize(linesCount);
    ulong actualBufferSize = m_pimpl->getBuff(&buffer, requestedBufferSize, true);
    if (actualBufferSize != requestedBufferSize) {
        setLastError(tr("Полученое кол-во байт (%1) меньше запрошенного (%2)").arg(actualBufferSize)
                                                                              .arg(requestedBufferSize));
        return false;
    }

    Frame frame;
    m_pimpl->convertRawToVector(buffer, frame, linesCount);
    m_pimpl->fixMatrixJoint(frame, linesCount);
    setLastCapturedFrame(frame);
    return true;
}

DeviceConfiguration SslDetector::defaultConfiguration() const
{
    DeviceConfiguration conf;
    conf.insert(TEST_MODE, false, tr("Тестовый режим"));
    conf.insert(CHARGE_TIME, 2.5, tr("Время накопления, мс [>0.0]"));
    conf.insert(PIXEL_WIDTH, 0.2, tr("Ширина пикселя, мм [>0.0]"));
    conf.insert(PIXEL_HEIGHT, 0.2, tr("Высота пикселя, мм [>0.0]"));
    conf.insert(GANE_MTR, 7, tr("Усиление матриц [>=0]"));
    conf.insert(JUNK_LINES, 300, tr("Мусорные строки [>=0]"));
    conf.insert(MATRIX_COUNT, 8, tr("Кол-во матриц [>0"));
    conf.insert(PIXELS_PER_MATRIX, 256, tr("Кол-во пикселей в матрице [>0]"));
    conf.insert(FIX_MATRIX_JOINT, true, tr("Коррекция стыков матриц в детекторе"));
    return conf;
}
