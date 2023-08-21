#include "MicDetector.h"
#include <QDataStream>
#include <QProcess>

#include <Device/DeviceLogging.h>
#include <json_rpc_debug_logger.h>

#include "../Common.h"

using namespace jcon;

const int TIMEOUT_SERVICE_MS = 30000;
const qreal DEFAULT_PIXEL_SIZE = 0.2643;
const qreal DEFAULT_CHARGE_TIME = 3.3;
const int DEFAULT_SERVICE_PORT = 11555;

const QString DETECTOR_CHARGE_TIME = QStringLiteral("main/chargeTime");
const QString DETECTOR_PIXEL_SIZE = QStringLiteral("main/pixel_size");
const QString SERVICE_PORT = QStringLiteral("main/service_port");

const QString LOAD_MIC_LIBRARY_COMMAND = QStringLiteral("loadMicLibrary");
const QString HARDWARE_INIT_COMMAND = QStringLiteral("hardwareInit");
const QString HARDWARE_SHUTDOWN_COMMAND = QStringLiteral("hardwareShutdown");
const QString PREPARE_READING_COMMAND = QStringLiteral("prepareReading");
const QString READ_HARDWAREINFO_COMMAND = QStringLiteral("readHardwareInfo");
const QString READ_DATA_COMMAND = QStringLiteral("readData");

struct MicDetector::PImpl
{
    QProcess *serviceProcess = nullptr;
    JsonRpcTcpClient *jconClient = nullptr;

    bool startService(QObject* parent, int servicePort);
    void stopService();

    bool connectToService(QObject* parent, int servicePort);
    void disconnectFromService();
};

bool MicDetector::PImpl::startService(QObject* parent, int servicePort)
{
    if (serviceProcess) {
        errDevice << "Service process already created";
        return false;
    }

    serviceProcess = new QProcess(parent);
    serviceProcess->setArguments(QStringList{QStringLiteral("-p"), QString::number(servicePort)});
#ifdef QT_DEBUG
    serviceProcess->setProgram(QStringLiteral("MicDetectorService//MicDetectorServiced.exe"));
#else
    serviceProcess->setProgram(QStringLiteral("MicDetectorService//MicDetectorService.exe"));
#endif
    serviceProcess->start();
    if (!serviceProcess->waitForStarted()) {
        errDevice << "Not start process " + serviceProcess->program();
        delete serviceProcess;
        serviceProcess = nullptr;
        return false;
    }
    return true;
}

void MicDetector::PImpl::stopService()
{
    if (serviceProcess) {
        serviceProcess->kill();
        serviceProcess->waitForFinished();
        delete serviceProcess;
        serviceProcess = nullptr;
    }
}

bool MicDetector::PImpl::connectToService(QObject *parent, int servicePort)
{
    if (jconClient && jconClient->isConnected()) {
        return true;
    }

    jconClient = new JsonRpcTcpClient(parent, std::make_shared<JsonRpcDebugLogger>(), TIMEOUT_SERVICE_MS);
    if (!jconClient->connectToServer("127.0.0.1", servicePort)) {
        errDevice << "Failed to connect to MicDetectorService";
        delete jconClient;
        jconClient = nullptr;
        return false;
    }
    return true;
}

void MicDetector::PImpl::disconnectFromService()
{
    if (jconClient) {
        delete jconClient;
        jconClient = nullptr;
    }
}

bool MicDetector::prepareDetector(int linesCount, qreal chargeTime)
{
    QVariantMap data;
    return callFunctionDetector(PREPARE_READING_COMMAND, data, linesCount, chargeTime);
}

bool MicDetector::readFrameDetector(Frame &frame)
{
    QVariantMap responseData;
    if (callFunctionDetector(READ_DATA_COMMAND, responseData)) {
        QByteArray stringScan = responseData.value(NAME_FIELD_FRAME).toByteArray();
        QByteArray data = QByteArray::fromBase64(stringScan);
        QDataStream steam(&data, QIODevice::ReadOnly);
        MicFrame micFrame;
        steam >> micFrame;

        infoDevice << "Frame size" << micFrame.size();

        if (micFrame.isEmpty()) {
            return false;
        }

        frame.clear();
        frame.resize(micFrame.size());
        for (int i = 0; i < micFrame.size(); ++i) {
            frame[i] = static_cast<float>(micFrame.at(i));
        }
    } else {
        return false;
    }

    return true;
}

bool MicDetector::processResponse(std::shared_ptr<jcon::JsonRpcResult> response, QString nameCommand, QVariantMap &data)
{
    if (response->isSuccess()) {
        QVariantMap responseData = response->result().toMap();
        if (!responseData.value(NAME_FIELD_IS_SUCCESS).toBool()) {
            errDevice << "Error command" << nameCommand;
            setLastError(responseData.value(NAME_FIELD_TEXT_ERROR).toString());
            return false;
        }

        data = responseData;
        return true;
    }

    errDevice << "Service responce not received";
    return false;
}

MicDetector::MicDetector(QObject *parent) : Detector(parent),
    m_pimpl(new PImpl)
{

}

template<typename... T>
bool MicDetector::callFunctionDetector(QString nameCommand, QVariantMap &responseData, T&&... args)
{
    auto response = m_pimpl->jconClient->call(nameCommand, std::forward<T>(args)...);
    return processResponse(response, nameCommand, responseData);
}

bool MicDetector::callFunctionDetector(QString nameCommand)
{
    QVariantMap data;
    return callFunctionDetector(nameCommand, data);
}

MicDetector::~MicDetector()
{

}

bool MicDetector::doTestConnection()
{
    const int countTestLines = 1;
    if (!prepareDetector(countTestLines, properties().chargeTimeMsec)) {
        return false;
    }

    Frame frame;
    return readFrameDetector(frame) && (frame.size() == countTestLines * properties().width);
}

bool MicDetector::doOpen()
{
    int port = currentConfiguration().value(SERVICE_PORT).toInt();
    if (!m_pimpl->startService(this, port)) {
        return false;
    }

    if (!m_pimpl->connectToService(this, port)) {
        m_pimpl->stopService();
        return false;
    }

    if (!callFunctionDetector(HARDWARE_INIT_COMMAND)) {
        m_pimpl->disconnectFromService();
        m_pimpl->stopService();
        return false;
    }

    QVariantMap responseHardwareInfo;
    if (!callFunctionDetector(READ_HARDWAREINFO_COMMAND, responseHardwareInfo)) {
        doClose();
        return false;
    }
    int widthDetectorPx = responseHardwareInfo.value(NAME_FIELD_WIDTH_DETECTOR).toInt();

    Properties props;
    auto &cfg = currentConfiguration();
    props.width = widthDetectorPx;
    props.chargeTimeMsec = cfg.value(DETECTOR_CHARGE_TIME).toReal();
    props.pixelSizeMm.setWidth(cfg.value(DETECTOR_PIXEL_SIZE).toReal());
    props.pixelSizeMm.setHeight(cfg.value(DETECTOR_PIXEL_SIZE).toReal());

    if (props.width < 1) {
        errDevice << "Width out of range";
        doClose();
        return false;
    }

    if (props.chargeTimeMsec <= 0) {
        warnDevice << "Charge time out of range. Using default: " << DEFAULT_CHARGE_TIME;
        props.chargeTimeMsec = DEFAULT_CHARGE_TIME;
    }

    if (props.pixelSizeMm.width() <= 0) {
        warnDevice << "Pixel width out of range. Using default: " << DEFAULT_PIXEL_SIZE;
        props.pixelSizeMm.setWidth(DEFAULT_PIXEL_SIZE);
    }

    props.pixelSizeMm.setHeight(props.pixelSizeMm.width());

    setProperties(props);

    return true;
}

void MicDetector::doClose()
{
    if (m_pimpl->jconClient) {
        callFunctionDetector(HARDWARE_SHUTDOWN_COMMAND);
    }

    m_pimpl->disconnectFromService();
    m_pimpl->stopService();
}

bool MicDetector::doPrepare()
{
    return prepareDetector(currentLines(), properties().chargeTimeMsec);
}

bool MicDetector::doCapture()
{
    Frame frame;

    {
        if (!readFrameDetector(frame)) {
            return false;
        }

        if (!frame.size()) {
           errDevice << "Frame empty";
           setLastError(tr("Получен пустой снимок"));
           return false;
        }
    }

    setLastCapturedFrame(frame);
    return true;
}

DeviceConfiguration MicDetector::defaultConfiguration() const
{
    DeviceConfiguration conf;
    conf.insert(DETECTOR_CHARGE_TIME, DEFAULT_CHARGE_TIME, tr("Время накопления, мс [>0.0]"));
    conf.insert(DETECTOR_PIXEL_SIZE, DEFAULT_PIXEL_SIZE, tr("Размер пикселя, мм [>0.0]"));
    conf.insert(SERVICE_PORT, DEFAULT_SERVICE_PORT, tr("Номер порта, который используется для связи "
                                                       "с сервисом детектора"));
    return conf;
}
