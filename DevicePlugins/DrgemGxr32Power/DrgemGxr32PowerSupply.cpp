#include "DrgemGxr32PowerSupply.h"

#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QVector>
#include <QElapsedTimer>

#include <Device/DeviceLogging.h>

#include "Gxr.h"
#include "GxrPackage.h"

const QString USE_SDK_GXR32 = QStringLiteral("main/use_sdk");
const QString UDP_PORT_SDK_GXR32 = QStringLiteral("main/udp_port");

const int TIMEOUT_READ_SOCKET_GXR_MS = 4000;
const int INTERVAL_CHECK_ANSWER_MS = 100;
const int MAX_TIME_PREPARE_MS = 5000;
const int INTERVAL_CHECK_PREPARE_MS = 200;

struct DrgemGxr32PowerSupply::PImpl
{
    bool useSdk;
    int portSdk;
    QUdpSocket *udpSocket = nullptr;
    PowerSupply::Results lastResult;
    bool islastResultReceivedSuccess = false;

    bool write(const GxrPackage &package);
    bool waitAnswer(GxrRegister command, GxrPackage* answer = nullptr);
    bool waitAnswer(const std::function<bool(const GxrPackage&)>& callback, int timeout = TIMEOUT_READ_SOCKET_GXR_MS);

    bool checkError(QString& error);
    bool isErrorOccurred(const GxrPackage &answerError, QString& error);
    bool isWarningOccurred(const GxrPackage &answerError, QString& warning);
    PowerSupply::Results calcRealKvMaTime(GxrPackage &p);
    GxrRegister indexTimeExp(int timeMs) const;
    GxrRegister indexMa(float amperage) const;
};

GxrRegister DrgemGxr32PowerSupply::PImpl::indexMa(float amperage) const
{
    QMapIterator<GxrRegister, float> it(TABLE_MA);
    if (!it.hasNext()) {
        return 0;
    }
    it.next();
    while (it.hasNext()) {
        auto itemStart = it;
        auto itemEnd = it.next();
        if (itemStart.value() <= amperage && itemEnd.value() >= amperage) {
            if (qAbs(itemStart.value() - amperage) < qAbs(itemEnd.value() - amperage)) {
                return itemStart.key();
            }
            return itemEnd.key();
        }
    }
    return 0;
}

GxrRegister DrgemGxr32PowerSupply::PImpl::indexTimeExp(int timeMs) const
{
    QMapIterator<GxrRegister, float> it(TABLE_TIME);
    if (!it.hasNext()) {
        return 0;
    }
    it.next();
    while (it.hasNext()) {
        auto itemStart = it;
        auto itemEnd = it.next();
        if ((itemStart.value() <= timeMs) && (itemEnd.value() >= timeMs)) {
            return itemEnd.key();
        }
    }
    return 0;
}

bool DrgemGxr32PowerSupply::PImpl::write(const GxrPackage &package)
{
    if (!udpSocket->isOpen()) {
        return false;
    }

    QByteArray buffer = package.toQByteArray();
    if (udpSocket->writeDatagram(buffer, QHostAddress::LocalHost, portSdk) != buffer.size()) {
        errDevice << "Not send data in GXR SDK";
        return false;
    }
    dbgDevice << "Write to udp socket GXR:" << buffer.toHex();
    return true;
}

bool DrgemGxr32PowerSupply::PImpl::waitAnswer(GxrRegister command, GxrPackage* answer)
{
    return waitAnswer([command, answer](const GxrPackage& package) {
        if (answer) {
            *answer = package;
        }
        return command == package.command();
    });
}

bool DrgemGxr32PowerSupply::PImpl::waitAnswer(const std::function<bool (const GxrPackage &)> &callback, int timeout)
{
    QByteArray data;
    QElapsedTimer timer;
    timer.start();
    while (timeout >= timer.elapsed()) {
        udpSocket->waitForReadyRead(INTERVAL_CHECK_ANSWER_MS);
        data += udpSocket->readAll();
        dbgDevice << "Buffer content udp socket GXR:" << data.toHex();

        while (data.size() >= GxrPackage::size) {
            QByteArray buffer = data.left(GxrPackage::size);
            data.remove(0, buffer.size());
            GxrPackage tmpPackage(buffer);

            if (!tmpPackage.isValid()) {
                errDevice << "Received not valid package: " << buffer.toHex();
                continue;
            }

            if (callback(tmpPackage)) {
                return true;
            }
        }
    }

    return false;
}

bool DrgemGxr32PowerSupply::PImpl::checkError(QString& error)
{
    if (!write(UDP_CMD_GET_ERROR)) {
        return false;
    }

    GxrPackage answer;
    if (!waitAnswer(UDP_CMD_UPDATE_ERROR, &answer)) {
        return false;
    }

    return isErrorOccurred(answer, error);
}

bool DrgemGxr32PowerSupply::PImpl::isErrorOccurred(const GxrPackage& answerError, QString &error)
{
    bool errorOccurred = false;
    QStringList listErrors;
    for (int i = 0; i < 3; ++i) {
        if (GxrRegister code = answerError.payload(i)) {
            errorOccurred = true;
            listErrors.append(
                        tr("Код ошибки РПУ E%1 %2").arg(QString::number(code, 16),
                                                         TABLE_ERROR.value(code, QStringLiteral("UNKNOWN"))));
        }
    }

    error += listErrors.join(" ");

    return errorOccurred;
}

bool DrgemGxr32PowerSupply::PImpl::isWarningOccurred(const GxrPackage &answerError, QString &warning)
{
    bool warningOccurred = false;
    if (GxrRegister code = answerError.payload(0)) {
        warningOccurred = true;
        warning = QStringLiteral("Warning GXR W%1 %2").arg(QString::number(code, 16),
                                                            TABLE_WARNING.value(code, QStringLiteral("UNKNOWN")));
    }

    return warningOccurred;
}

PowerSupply::Results DrgemGxr32PowerSupply::PImpl::calcRealKvMaTime(GxrPackage &p)
{
    PowerSupply::Results params;
    params.voltageKV = (p.payload(1) * 256 + p.payload(0)) / 10.0;
    params.amperageMA = (p.payload(3) * 256 + p.payload(2)) / 10.0;
    params.exposureMs = ((p.payload(6) * 512 ) + (p.payload(5) * 256 ) + p.payload(4)) / 10.0;
    return params;
}

DrgemGxr32PowerSupply::DrgemGxr32PowerSupply(QObject *parent):
    PowerSupply(parent),
    m_pimpl(new PImpl)
{

}

DrgemGxr32PowerSupply::~DrgemGxr32PowerSupply()
{

}

bool DrgemGxr32PowerSupply::doOpen()
{
    auto &cfg = currentConfiguration();

    m_pimpl->useSdk = cfg.value(USE_SDK_GXR32).toBool();
    if (!m_pimpl->useSdk) {
        return true;
    }

    m_pimpl->portSdk = cfg.value(UDP_PORT_SDK_GXR32).toInt();
    if (m_pimpl->udpSocket) {
        delete m_pimpl->udpSocket;
    }
    m_pimpl->udpSocket = new QUdpSocket(this);
    m_pimpl->udpSocket->bind(QHostAddress::LocalHost, m_pimpl->portSdk + 1);
    m_pimpl->udpSocket->connectToHost(QHostAddress::LocalHost, m_pimpl->portSdk);
    if (!m_pimpl->udpSocket->waitForConnected()) {
        setLastError(tr("Нет подключения к GXR SDK"));
        return false;
    }

    m_pimpl->write(UDP_CMD_SET_CONNECT);

    if (!m_pimpl->waitAnswer([](const GxrPackage& package) {
            return package.command() == UDP_CMD_UPDATE_RKVMATIME ||
                   package.command() == UDP_CMD_UPDATE_XRAY_EXP_CTRL;
    })) {
        setLastError(tr("Нет подключения к GXR SDK"));
        errDevice << "Open:"
                  << "Failed set connect GXR SDK";
    }

    infoDevice << "Open"
               << "Connect GXR SDK success";
    return true;
}

void DrgemGxr32PowerSupply::doClose()
{
    if (m_pimpl->udpSocket) {
        m_pimpl->udpSocket->disconnectFromHost();
        m_pimpl->udpSocket->waitForDisconnected();
        delete m_pimpl->udpSocket;
    }
}

bool DrgemGxr32PowerSupply::doTestConnection()
{
    if (!m_pimpl->useSdk) {
        return true;
    }

    m_pimpl->write(UDP_CMD_ECHO);
    return m_pimpl->waitAnswer(UDP_CMD_ECHO);
}

bool DrgemGxr32PowerSupply::doPrepare()
{
    if (!m_pimpl->useSdk) {
        return true;
    }

    m_pimpl->islastResultReceivedSuccess = false;

    auto &params = currentParams();

    m_pimpl->write(UDP_CMD_SET_RESET);
    GxrPackage answerSetReset;
    auto func = [&answerSetReset](const GxrPackage& package) {
        answerSetReset = package;
        return package.command() == UDP_CMD_UPDATE_APR ||
               package.command() == UDP_CMD_UPDATE_BUCKY ||
               package.command() == UDP_CMD_UPDATE_AEC ||
               package.command() == UDP_CMD_UPDATE_RKVMATIME ||
               package.command() == UDP_CMD_UPDATE_WARNING ||
               package.command() == UDP_CMD_UPDATE_ERROR;
    };
    if (m_pimpl->waitAnswer(func)) {
        QString warning;
        if (m_pimpl->isWarningOccurred(answerSetReset, warning)) {
            warnDevice << warning;
        }

        QString error;
        if (m_pimpl->isErrorOccurred(answerSetReset, error)) {
            errDevice << error;
            return false;
        }

    } else {
        errDevice << "Prepare:"
                  << "Failed command reset";
        return false;
    }

    const int POS_KV = 0;
    const int POS_INDEX_MA = 1;
    const int POS_INDEX_TIME = 2;

    GxrPackage commandSetParam(UDP_CMD_SET_RKVMATIM);
    commandSetParam.setPayload(POS_KV, static_cast<GxrRegister>(params.voltageKV));
    commandSetParam.setPayload(POS_INDEX_MA, m_pimpl->indexMa(params.amperageMA));
    commandSetParam.setPayload(POS_INDEX_TIME, m_pimpl->indexTimeExp(params.exposureMs));

    infoDevice << "Prepare:"
               << QString("Set KV %1 index MA %2 index TIME %3").arg(commandSetParam.payload(POS_KV))
                                                                .arg(commandSetParam.payload(POS_INDEX_MA))
                                                                .arg(commandSetParam.payload(POS_INDEX_TIME));

    m_pimpl->write(commandSetParam);
    GxrPackage answerSetParam;
    if (!m_pimpl->waitAnswer(UDP_CMD_UPDATE_RKVMATIME, &answerSetParam)) {
        errDevice << "Prepare:"
                  << "Failed set param scans";
        return false;
    } else {
        infoDevice << "Prepare:"
                   << QString("Returned param KV %1 index MA %2 index TIME %3").arg(answerSetParam.payload(POS_KV))
                                                                               .arg(answerSetParam.payload(POS_INDEX_MA))
                                                                               .arg(answerSetParam.payload(POS_INDEX_TIME));
        for (int i = 0; i < 3; ++i) {
            if (commandSetParam.payload(i) != answerSetParam.payload(i)) {
                setLastError(tr("Выбранные параметры сьемки не могут быть уставлены в РПУ"));
                return false;
            }
        }
    }

   for (int i = 0; i < MAX_TIME_PREPARE_MS / INTERVAL_CHECK_PREPARE_MS; i++) {
        m_pimpl->write(UDP_CMD_GET_XRAY_EXP_CTRL);
        GxrPackage answer;
        if (m_pimpl->waitAnswer(UDP_CMD_UPDATE_XRAY_EXP_CTRL, &answer) && answer.payload(0)) {
            infoDevice << "Prepare: success";
            break;
        }
        QThread::msleep(INTERVAL_CHECK_PREPARE_MS);

        m_pimpl->write(UDP_CMD_GET_ERROR);
        QString error;
        if (m_pimpl->checkError(error)) {
            setLastError(error);
            return false;
        }
    }

    return true;
}

bool DrgemGxr32PowerSupply::doOn()
{
    return true;
}

bool DrgemGxr32PowerSupply::doOff()
{
    return true;
}

bool DrgemGxr32PowerSupply::doWaitForError()
{
    if (!m_pimpl->useSdk) {
        return PowerSupply::doWaitForError();
    }

    GxrPackage answer;
    auto func = [&answer](const GxrPackage& package) {
        answer = package;
        return package.command() == UDP_CMD_UPDATE_ERROR ||
               package.command() == UDP_CMD_UPDATE_FB_RKVMATIME;
    };

    if (m_pimpl->waitAnswer(func, currentParams().exposureMs + TIMEOUT_READ_SOCKET_GXR_MS)) {
        QString error;
        switch (answer.command()) {
        case UDP_CMD_UPDATE_ERROR:
            if (m_pimpl->isErrorOccurred(answer, error)) {
                setLastError(error);
                return true;
            }
            break;
        case UDP_CMD_UPDATE_FB_RKVMATIME:
            m_pimpl->lastResult = m_pimpl->calcRealKvMaTime(answer);
            m_pimpl->islastResultReceivedSuccess = true;
            return false;
        }
    }

    return false;
}

bool DrgemGxr32PowerSupply::doGetResults(Results &results)
{
    if (!m_pimpl->useSdk) {
        const auto params = currentParams();
        results.voltageKV = params.voltageKV;
        results.amperageMA = params.amperageMA;
        return true;
    }

    if (m_pimpl->islastResultReceivedSuccess) {
        results = m_pimpl->lastResult;
    } else {
        errDevice << "Get Result:"
                  << "Real x-ray on parameters were not received";
        return false;
    }

    return true;
}

DeviceConfiguration DrgemGxr32PowerSupply:: defaultConfiguration() const
{
    DeviceConfiguration conf;
    conf.insert(USE_SDK_GXR32, false, tr("Использовать SDK GXR32 (необходимо подключение пульта к ПК)"));
    conf.insert(UDP_PORT_SDK_GXR32, 5000, tr("Порт подключения к SDK GXR32"));
    return conf;
}
