#include "IstramonoPowerSupply.h"

#include <QVector>
#include <QThread>

#include <Device/DeviceLogging.h>
#include <Device/NpFrame.h>

const QString FETCH_EXPOSURE_PARAM = QStringLiteral("main/fetch_exposure");
const QString USE_PHYSICAL_BUTTONS_PARAM = QStringLiteral("main/use_physical_buttons");

IstramonoPowerSupply::IstramonoPowerSupply(QObject *parent) : PowerSupply(parent),
    m_accessor(nullptr)
{

}

QString IstramonoPowerSupply::decodeError(int errCode) const
{
    if (!errCode) {
        return QString();
    }
    
    const QMap<int, QString> errors = {
        {1, tr("Отсутствует связь с компьютером")},
        {2, tr("Сработал термоконтакт рентгеновского излучателя")},
        {3, tr("Молекулярный накопитель энергии разряжен")},
        {4, tr("Рентгеновская трубка перегрета")},
        {10, tr("Не работает схема аппаратного таймера снимка. TST = 0")},
        {11, tr("Не работает схема аппаратного таймера снимка. TST = 1")},
        {12, tr("Снимок закончился по ошибке инвертора")},
        {13, tr("Силовой инвертор не готов к включению")},
        {14, tr("Снимок закончился по сигналу аппаратного таймера")},
        {15, tr("Время снимка превысило 150% от расчета. Требуется перекалибровка рентгеновской трубки")},
        {16, tr("Анодный ток превысил допуск")},
        {17, tr("Анодный ток ниже допуска")},
        {18, tr("Сигнал разрешения снимка присутствует до начала снимка")},
        {19, tr("Батарея накопительных конденсаторов разражена")},
        {21, tr("Ток накала трубки превысил максимально допустимое значение")},
        {22, tr("Ток накала трубки больше допуска 110% от уставки")},
        {23, tr("Ток накала трубки меньше допуска 90 % от уставки")},
        {24, tr("Утрачены данные конфигурации. Восстановлены заводские значения")},
        {25, tr("Утрачены данные калибровки трубки")},
        {26, tr("Нарушена таблица органавтоматики")},
        {27, tr("Не работает схема вращения анода")},
        {28, tr("Анодное напряжение превысило допуск 110% от уставки")},
        {29, tr("Анодное напряжение ниже допуска 90% от уставки")},
        {30, tr("Сработала защита силового инвертора. Выключите и включите аппарат")},
        {33, tr("Рентгеноская трубка перегрета")},
        {35, tr("Дверь кабины не закрыта")},
        {41, tr("Рентгеновский излучатель перегрет")},
        {42, tr("Нет сигнала с датчика накала трубки")},
        {44, tr("При включении аппарата нажата кнопка подготовки")},
        {45, tr("При включении аппарата нажата кнопка снимка")},
        {46, tr("Сбой в обмене информацией")},
        {47, tr("Переполнение приемного буфера")},
        {48, tr("Перекос токов")},
        {51, tr("В течении 20 мс нет фронта анодного напряжения")},
        {52, tr("Питание +5В платы не в норме (5%)")},
        {53, tr("Питание AVDD +5В платы не в норме (5%)")},
        {54, tr("Перекос питания +12В и -12В платы (5%)")},
        {55, tr("Не нажата ни одна педаль")},
        {56, tr("Схема защиты от включения рентгена без движения заблокирована. "
                "Возможно не исправен датчик числа оборотов")},
        {57, tr("Подготовка прервана")},
        {58, tr("Стабилизация тока анода включена до начала снимка")},
        {59, tr("Ток накала трубки превысил макcимально допустимое значение, снимок прерван")}
    };
    
    if (errors.contains(errCode)) {
        return tr("%1 (%2)").arg(errors.value(errCode)).arg(errCode);
    }

    return tr("Неизвестная ошибка (%1)").arg(errCode);
}

IstramonoPowerSupply::~IstramonoPowerSupply()
{

}

bool IstramonoPowerSupply::doOpen()
{
    m_accessor = dispatcher().getAccessor(0x10);

    if (!m_accessor) {
        errDevice << "Open:"
                  << "Failed to get dispatcher accessor";
        setLastError(tr("Не удалось получить доступ к диспетчеру"));
        return false;
    }

    return true;
}

void IstramonoPowerSupply::doClose()
{

}

bool IstramonoPowerSupply::doTestConnection()
{
    NpFrame pingRequest(0x10, 0x1, 0x55, 0x55, 0x55);
    NpFrame pingRightResponse(0x10, 0x2, 0xAA, 0xAA, 0xAA);
    NpFrame pingResponse = m_accessor->writeAndRead(pingRequest);
    if (pingResponse != pingRightResponse) {
        errDevice << "Test connection:"
                  << "Ping response:" << pingResponse.toHex()
                  << "Expected first" << NpFrame::size << "bytes:" << pingRightResponse.toHex();
        setLastError(tr("Ответ на команду тест связи отсутствует или неверный"));
        return false;
    }

    return true;
}

bool IstramonoPowerSupply::doPrepare()
{
    auto &params = currentParams();

    const auto voltage = static_cast<quint8>(params.voltageKV);
    const auto amperage = static_cast<quint8>(params.amperageMA);
    const auto exposure = static_cast<quint16>(params.exposureMs);

    if (voltage < 40 || voltage > 160) {
        errDevice << "Prepare:"
                  << "Voltage:" << voltage
                  << "Expected: 40 <= V <= 160";
        setLastError(tr("Неверное значение напряжения"));
        return false;
    }

    if (amperage < 5 || amperage > 120) {
        errDevice << "Prepare:"
                  << "Amperage:" << amperage
                  << "Expected: 5 <= A <= 120";
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

    NpFrame setExposureRequest(0x10, 0x8,
                               static_cast<quint8>(exposure & 0xFF),
                               static_cast<quint8>(exposure >> 8), 0);
    if (!m_accessor->write(setExposureRequest)) {
        errDevice << "Prepare:"
                  << "Failed to send set exposure command";
        return false;
    }

    NpFrame setParamsRequest(0x10, 0xAC, voltage, amperage);
    NpFrame setParamsRightResponse(0x10, 0xAB, voltage, amperage);
    NpFrame setParamsResponse = m_accessor->writeAndRead(setParamsRequest);

    if (setParamsResponse != setParamsRightResponse) {
        errDevice << "Prepare:"
                  << "Response is invalid:" << setParamsResponse.toHex();
        setLastError(tr("Ответ на команду установки параметров неверный или отсутствует"));
        return false;
    }

    NpFrame prepareRightResponse(0x10, 0xAE);
    const int timeoutPrepareResponse = 8000;
    NpFrame prepareResponse;
    if (currentConfiguration().value(USE_PHYSICAL_BUTTONS_PARAM).toBool()) {
        prepareResponse = m_accessor->read(timeoutPrepareResponse);
    } else {
        NpFrame pressButtonPrepare(0x10, 0x7B, 1);
        m_accessor->write(pressButtonPrepare);
        NpFrame pressButtonScan(0x10, 0x7A, 1);
        prepareResponse = m_accessor->writeAndRead(pressButtonScan, timeoutPrepareResponse);
    }

    if (!prepareResponse.isValid()) {
        setLastError(tr("Превышено время ожидания ответа на команду подготовки"));
        return false;
    }

    if (prepareResponse != prepareRightResponse) {
        errDevice << "Prepare:"
                  << "Response:" << prepareResponse.toHex()
                  << "Expected:" << prepareRightResponse.toHex();

        const auto address = prepareResponse.addr();
        const auto command = prepareResponse.cmd();

        if (address == 0x10 && (command == 0x71 || command == 0x72)) {
            setLastError(decodeError(prepareResponse.reg1()));
            return false;
        }

        setLastError(tr("Ответ на команду подготовки неверный"));
        return false;
    }

    return true;
}

bool IstramonoPowerSupply::doOn()
{
    NpFrame turnOnXrayRequest(0x10, 0x1C);
    if (!m_accessor->write(turnOnXrayRequest)) {
        setLastError(tr("Не удалось отправить команду \"Снимок\""));
        return false;
    }

    return true;
}

bool IstramonoPowerSupply::doWaitForError()
{
    QByteArray statusResponse = m_accessor->read(currentParams().exposureMs);
    if (statusResponse.isEmpty()) {
        statusResponse = m_accessor->writeAndRead(NpFrame(0x10, 0x70));
    }

    QVector<NpFrame> frames = NpFrame::splitFrames(statusResponse);
    const NpFrame statusResponseFrame = !frames.isEmpty() ? frames.last() : NpFrame();

    if (statusResponseFrame.isValid()) {
        const auto address = statusResponseFrame.addr();
        const auto command = statusResponseFrame.cmd();
        const auto code = statusResponseFrame.reg1();

        if (address == 0x10) {
            switch (command) {
            case 0x71:
            case 0x72:
                if (!code) {
                    return false;
                }

                setLastError(decodeError(code));
                return true;
            case 0x1E:
                return false;
            }
        }
    }

    setLastError(tr("Неизвестная ошибка"));
    return true;
}

bool IstramonoPowerSupply::doOff()
{
    NpFrame turnOffXrayRequest(0x10, 0xBA);
    if (!m_accessor->write(turnOffXrayRequest)) {
        setLastError(tr("Не удалось отправить команду \"Отмена снимка\""));
        return false;
    }

    if (!currentConfiguration().value(USE_PHYSICAL_BUTTONS_PARAM).toBool()) {
        NpFrame releaseButtonPrepare(0x10, 0x7B);
        if (!m_accessor->write(releaseButtonPrepare)) {
            setLastError(tr("Не удалось отправить команду \"Отпустить кнопку подготовка\""));
            return false;
        }

        NpFrame releaseButtonScan(0x10, 0x7A);
        if (!m_accessor->write(releaseButtonPrepare)) {
            setLastError(tr("Не удалось отправить команду \"Отпустить кнопку снимок\""));
            return false;
        }
    }

    return true;
}

bool IstramonoPowerSupply::doGetResults(Results &results)
{
    NpFrame measureRequest(0x10, 0x77);
    const NpFrame measureResponse = m_accessor->writeAndRead(measureRequest);
    if (!measureResponse.isValid()
        || measureResponse.addr() != 0x10
        || measureResponse.cmd() != 0x78) {
        errDevice << "Received response for measured params is invalid:" << measureResponse.toHex();
        setLastError(tr("Ответ на команду получения измеренных результатов отсутствует или неверный"));
        return false;
    }

    if (currentConfiguration().value(FETCH_EXPOSURE_PARAM).toBool()) {
        NpFrame exposureRequest(0x10, 0x73);
        const NpFrame exposureResponse = m_accessor->writeAndRead(exposureRequest);
        if (!exposureResponse.isValid()
            || exposureResponse.addr() != 0x10
            || exposureResponse.cmd() != 0x74) {
            errDevice << "Received response for exposure param is invalid:" << exposureResponse.toHex();
            setLastError(tr("Ответ на команду измеренного времени экспозиции отсутствует или неверный"));
            return false;
        }
        
        results.exposureMs = (static_cast<quint16>(exposureResponse.reg2()) << 8) |
                             (static_cast<quint16>(exposureResponse.reg1()));
    }

    results.voltageKV  = measureResponse.reg1();
    results.amperageMA = ((static_cast<quint16>(measureResponse.reg3()) << 8) |
                          (static_cast<quint16>(measureResponse.reg2()))) / 10.0;
    return true;
}

DeviceConfiguration IstramonoPowerSupply::defaultConfiguration() const
{
    DeviceConfiguration config;
    config.insert(FETCH_EXPOSURE_PARAM, true, tr("Запрашивать измеренное время экспозиции"));
    config.insert(USE_PHYSICAL_BUTTONS_PARAM, true, tr("Использовать для включения рентгена пульт с кнопками"));
    return config;
}

bool IstramonoPowerSupply::doReset()
{
    if (!m_accessor->write(NpFrame(0x10, 0x21))) {
        setLastError(tr("Не удалось отправить команду \"Сброс УРПС\""));
        return false;
    }

    return true;
}
