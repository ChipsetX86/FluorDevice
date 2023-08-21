#include "Np43PowerSupply.h"

#include <QFile>

#include <Settings/LocalSettings.h>
#include <Device/DeviceLogging.h>
#include <Device/Dispatcher.h>
#include <Device/NpFrame.h>

#include "Np43.h"

const QString PATH_FILE_WITH_TABLE_CURRENT_FILAMENT_TUBE = QStringLiteral("main/path_file_table_current");
const int WIDTH_TABLE_CURRENT = 20;
const int HEIGHT_TABLE_CURRENT = 33;
const ushort MIN_CURRENT_FILAMENT_TUBE = 200;
const ushort MAX_CURRENT_FILAMENT_TUBE = 900;
const ushort STEP_KV_IN_TABLE = 5;
const ushort STEP_MA_IN_TABLE = 3;
const ushort MIN_KV_IN_TABLE_CURRENT = 40;
const ushort MIN_MA_IN_TABLE_CURRENT = STEP_MA_IN_TABLE;
const int MAX_TIME_PREPARE_MS = 5000;
const int MAX_TIME_WAIT_RESULT_SCAN_MS = 3000;
const int TIMEOUT_TEST_CONNECTION = 12000;

namespace {
    quint8 getBit(qint8 val, qint8 number)
    {
        return (val >> number) & 1;
    }
}

struct Np43PowerSupply::PImpl
{
    PowerSupply::Results lastResult;
    bool islastResultReceivedSuccess = false;
    ushort tableCurrentFilamentTube[WIDTH_TABLE_CURRENT][HEIGHT_TABLE_CURRENT];
    Dispatcher::Accessor *m_accessorPower = nullptr;
    Dispatcher::Accessor *m_accessorBku = nullptr;

    bool loadTableCurrentFilamentTube(const QString &fullPathFile);
    ushort calcCurrentFilamentTube(quint8 kV, quint8 mA);
    bool isErrorOccured(const NpFrame &frame, QString& error);
    bool checkCurrentFilamentTube(ushort current);
};

bool Np43PowerSupply::PImpl::loadTableCurrentFilamentTube(const QString &fullPathFile)
{
    QFile file(fullPathFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
         return false;
    }

    std::fill_n(&tableCurrentFilamentTube[0][0],
                WIDTH_TABLE_CURRENT*HEIGHT_TABLE_CURRENT,
                MIN_CURRENT_FILAMENT_TUBE);

    QTextStream in(&file);
    QRegExp rx("(\\d+)");
    ushort row = 0;
    while (!in.atEnd()) {
        QString line = in.readLine();
        int pos = 0;
        ushort column = 0;
        if (row >= HEIGHT_TABLE_CURRENT) {
            break;
        }

        while ((pos = rx.indexIn(line, pos)) != -1) {
            if (column < WIDTH_TABLE_CURRENT) {
                ushort current = rx.cap(1).toUShort();
                if (!checkCurrentFilamentTube(current)) {
                    errDevice << "Wrong tube filament current:" << current;
                } else {
                    tableCurrentFilamentTube[column][row] = current;
                }
            }
            pos += rx.matchedLength();
            column++;
        }
        row++;
    }
    file.close();
    return true;
}

ushort Np43PowerSupply::PImpl::calcCurrentFilamentTube(quint8 kV, quint8 mA)
{
    int row = qRound(qMax(0, mA - MIN_MA_IN_TABLE_CURRENT) * 1.f / STEP_MA_IN_TABLE);
    int column = kV < MIN_KV_IN_TABLE_CURRENT ? 0 :
                     qRound(qMax(0, kV - MIN_KV_IN_TABLE_CURRENT) * 1.f / STEP_KV_IN_TABLE) + 1;

    row = qMin(row, HEIGHT_TABLE_CURRENT - 1);
    column = qMin(column, WIDTH_TABLE_CURRENT - 1);

    return tableCurrentFilamentTube[column][row];
}

bool Np43PowerSupply::PImpl::isErrorOccured(const NpFrame &frame, QString &error)
{
    const quint8 address = frame.addr();
    const int codeError = frame.cmd() - 0x70;
    if (address != 0x10 || codeError < 0 || codeError > WIDTH_TABLE_ERROR) {
        return false;
    }

    const quint8 P2_DT2 = frame.reg1();
    const quint8 P2_DT1 = frame.reg2();
    const quint8 P1_DT = frame.reg3();

    QStringList listErrorPower;
    for (int i = 0; i < HEIGHT_TABLE_ERROR; i++) {
        if (getBit(P1_DT, i) != TABLE_ERROR_P1_DT[i][codeError]) {
            listErrorPower.append(ERROR_P1_DT[i]);
        }
        if (getBit(P2_DT1, i) != TABLE_ERROR_P2_DT1[i][codeError]) {
            listErrorPower.append(ERROR_P2_DT1[i]);
        }
        if (getBit(P2_DT2, i) != TABLE_ERROR_P2_DT2[i][codeError]) {
            listErrorPower.append(ERROR_P2_DT2[i]);
        }
    }

    error = tr("РПУ Ошибка работы, код ответа состояния %1 Регистры: %2 %3 %4 Ошибки: %5").
            arg(QString::number(codeError),
                QString::number(P2_DT2),
                QString::number(P2_DT1),
                QString::number(P1_DT),
                listErrorPower.join(" "));
    return true;
}

bool Np43PowerSupply::PImpl::checkCurrentFilamentTube(ushort current)
{
    return current >= MIN_CURRENT_FILAMENT_TUBE && current <= MAX_CURRENT_FILAMENT_TUBE;
}

Np43PowerSupply::Np43PowerSupply(QObject *parent):
    PowerSupply(parent), m_pimpl(new PImpl)
{

}

Np43PowerSupply::~Np43PowerSupply()
{

}

DeviceConfiguration Np43PowerSupply::defaultConfiguration() const
{
    DeviceConfiguration config;
    config.insert(PATH_FILE_WITH_TABLE_CURRENT_FILAMENT_TUBE,
                  LocalSettings::instance().dirPath() + QStringLiteral("/rentgen.wrd"),
                  tr("Имя файла с токами накала рентгеновской трубки"));
    return config;
}

bool Np43PowerSupply::doOpen()
{
    m_pimpl->m_accessorPower = dispatcher().getAccessor(0x10);
    m_pimpl->m_accessorBku = dispatcher().getAccessor(0x25);

    if (!m_pimpl->m_accessorPower || !m_pimpl->m_accessorBku) {
        errDevice << "Open:"
                  << "Failed to get dispatcher accessor";
        setLastError(tr("Не удалось получить доступ к диспетчеру"));
        return false;
    }

    QString path = currentConfiguration().value(PATH_FILE_WITH_TABLE_CURRENT_FILAMENT_TUBE).toString();
    if (!QFile::exists(path)) {
        path = QStringLiteral(":/rentgen.wrd");
    }

    if (!m_pimpl->loadTableCurrentFilamentTube(path)) {
        setLastError(tr("Не удалось загрузить таблицу с токами накала из файла \"%1\".").arg(path));
        return false;
    }

    return true;
}

void Np43PowerSupply::doClose()
{
    NpFrame powerOff(0x10, 0x33);
    m_pimpl->m_accessorPower->write(powerOff);
}

bool Np43PowerSupply::doTestConnection()
{
    NpFrame pingRequest(0x10, 0x1, 0x55, 0x55, 0x55);
    NpFrame pingRightResponse(0x10, 0x2, 0xAA, 0xAA, 0xAA);

    QElapsedTimer timer;
    timer.start();
    while (TIMEOUT_TEST_CONNECTION >= timer.elapsed()) {
        const NpFrame pingResponse = m_pimpl->m_accessorPower->writeAndRead(pingRequest);
        if (pingResponse != pingRightResponse) {
            errDevice << "Test connection:"
                      << "Ping response:" << pingResponse.toHex()
                      << "Expected first" << NpFrame::size << "bytes:" << pingRightResponse.toHex();
        } else {
            return true;
        }
    }

    setLastError(tr("Ответ на команду тест связи отсутствует или неверный"));
    return false;
}

bool Np43PowerSupply::doPrepare()
{
    auto &params = currentParams();

    m_pimpl->islastResultReceivedSuccess = false;

    const auto voltage = static_cast<quint8>(params.voltageKV);
    const auto amperage = static_cast<quint8>(params.amperageMA);
    const auto exposure = static_cast<quint16>(params.exposureMs);

    if (voltage < 40 || voltage > 125) {
        errDevice << "Prepare:"
                  << "Voltage:" << voltage
                  << "Expected: 40 <= V <= 125";
        setLastError(tr("Неверное значение напряжения"));
        return false;
    }

    if (amperage < 3 || amperage > 100) {
        errDevice << "Prepare:"
                  << "Amperage:" << amperage
                  << "Expected: 3 <= A <= 100";
        setLastError(tr("Неверное значение тока"));
        return false;
    }

    if (exposure < 1 || exposure > 20000) {
        errDevice << "Prepare:"
                  << "Exposure:" << exposure
                  << "Expected: 1 <= E <= 20000";
        setLastError(tr("Неверное значение времени экспозиции"));
        return false;
    }

    const ushort currentFilamentTube = m_pimpl->calcCurrentFilamentTube(voltage, amperage);
    dbgDevice << "Select current filament tube" << currentFilamentTube;

    NpFrame setParamsRequest(0x10, 0xAC, voltage,
                             static_cast<quint8>(currentFilamentTube & 0xFF),
                             static_cast<quint8>((currentFilamentTube >> 8) & 0xFF));
    if (!m_pimpl->m_accessorPower->write(setParamsRequest)) {
        errDevice << "Prepare:"
                  << "Failed to send set exposure command";
        return false;
    }

    NpFrame pushPrepareButton(0x25, 0xAD);
    NpFrame prepareRightResponse(0x10, 0xAE);
    m_pimpl->m_accessorBku->write(pushPrepareButton);
    const NpFrame pushPrepareResponse = m_pimpl->m_accessorPower->read(MAX_TIME_PREPARE_MS);
    if (pushPrepareResponse != prepareRightResponse) {
        QString error;
        if (pushPrepareResponse.isValid() &&
                m_pimpl->isErrorOccured(pushPrepareResponse, error)) {
            setLastError(error);
        } else {
            errDevice << "Prepare:"
                      << "Response is invalid:" << pushPrepareResponse.toHex();
            setLastError(tr("Ответ на команду установки параметров неверный или отсутствует"));
        }
        return false;
    }

    NpFrame stateCommand(0x10, 0x4D);
    const NpFrame stateResponse = m_pimpl->m_accessorPower->writeAndRead(stateCommand);
    if (!stateResponse.isValid()
        || stateResponse.addr() != 0x10
        || stateResponse.cmd() != 0x5D) {
        errDevice << "Prepare:"
                  << "Response is invalid:" << stateResponse.toHex();
        setLastError(tr("Ответ на запрос состояния РПУ неверный или отсутствует"));
        return false;
    }

    return true;
}

bool Np43PowerSupply::doOn()
{
    auto &params = currentParams();
    const auto amperage = static_cast<quint8>(params.amperageMA);
    const auto exposure = static_cast<quint16>(params.exposureMs);

    NpFrame turnOnXrayRequest(0x10, 0x1C, amperage,
                              static_cast<quint8>(exposure & 0xFF),
                              static_cast<quint8>(exposure >> 8));
    if (!m_pimpl->m_accessorPower->write(turnOnXrayRequest)) {
        setLastError(tr("Не удалось отправить команду \"Снимок\""));
        return false;
    }

    return true;
}

bool Np43PowerSupply::doWaitForError()
{
    QElapsedTimer timer;
    timer.start();
    int timeout = MAX_TIME_WAIT_RESULT_SCAN_MS + currentParams().exposureMs;
    while (timeout >= timer.elapsed()) {
        NpFrame stateAnswer(0x10, 0x4D);
        const auto stateResponse = m_pimpl->m_accessorPower->writeAndRead(stateAnswer);
        for (const auto &response: NpFrame::splitFrames(stateResponse)) {
            if (!response.isValid()) {
                errDevice << "Wait for error:"
                          << "Response is invalid:" << response.toHex();
                setLastError(tr("Ответ на команду запроса состояния неверная или отсуствует"));
                return true;
            }

            QString error;
            if (m_pimpl->isErrorOccured(response, error)) {
                setLastError(error);
                return true;
            }

            if (response.addr() == 0x10 && response.cmd() == 0x1E) {
                m_pimpl->lastResult.voltageKV = response.reg1() * 0.8f;
                m_pimpl->lastResult.amperageMA = response.reg2() * 0.4f;
                m_pimpl->lastResult.exposureMs = timer.elapsed();
                m_pimpl->islastResultReceivedSuccess = true;
                break;
            }
        }

        if (m_pimpl->islastResultReceivedSuccess) {
            break;
        }
    }

    return false;
}

bool Np43PowerSupply::doOff()
{
    return true;
}

bool Np43PowerSupply::doGetResults(Results &results)
{
    if (m_pimpl->islastResultReceivedSuccess) {
        results = m_pimpl->lastResult;
    } else {
        errDevice << "Get Result:"
                  << "Real x-ray on parameters were not received";
        return false;
    }

    return true;
}
