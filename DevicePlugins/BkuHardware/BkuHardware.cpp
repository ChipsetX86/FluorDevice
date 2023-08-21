#include "BkuHardware.h"

#include <QVector>
#include <QThread>
#include <QElapsedTimer>

#include <Device/NpFrame.h>

const QString RACK_MOVING = QStringLiteral("main/rack_moving");
const QString RACK_UP_SPEED = QStringLiteral("rack_up_speed");
const QString RACK_DOWN_SPEED = QStringLiteral("rack_down_speed");
const QString RACK_MOVE_TIMEOUT = QStringLiteral("rack_move_timeout");
const QString DOOR_MOVE_TIMEOUT = QStringLiteral("door_move_timeout");

BkuHardware::BkuHardware(QObject *parent) : Hardware(parent),
    m_enableMovingRack(true),
    m_rack_move_timeout(0),
    m_door_move_timeout(0)
{

}

BkuHardware::~BkuHardware()
{

}

bool BkuHardware::doTestConnection()
{
    NpFrame testCommand(0x25, 0x1, 0x55, 0x55, 0x55);
    NpFrame rightAnswer(0x25, 0x2, 0xAA, 0xAA, 0xAA);

    if (m_accessor->writeAndRead(testCommand, 5000) != rightAnswer) {
        setLastError(tr("Неверный ответ или превышено время ожидания"));
        return false;
    }

    return true;
}

bool BkuHardware::checkState(const CheckStateCallback &callback, int timeout, CancelationTokenSource::Token token)
{
    QElapsedTimer timer;
    timer.start();

    do {
        refreshState();
        if (callback()) {
            return true;
        }
    } while (!token.isCanceled() && !timer.hasExpired(timeout));

    return false;
}

bool BkuHardware::clearMechanicsRegs()
{
    if (!m_accessor->write(NpFrame(0x25, 0x3D, 0xFF, 0xFF, 0xFF))) {
        setLastError(tr("Не удалось очистить регистры управления механикой"));
        return false;
    }

    return true;
}

quint8 BkuHardware::speedRackScanDownCommand() const
{
    switch (currentConfiguration().value(RACK_DOWN_SPEED).toInt()) {
    case 1:
        return 0xFB;
    case 2:
        return 0xEB;
    case 3:
        return 0x7B;
    case 4:
        return 0x6B;
    default:
        return 0xFF;
    }
}

quint8 BkuHardware::speedRackScanUpCommand() const
{
    switch (currentConfiguration().value(RACK_UP_SPEED).toInt()) {
    case 1:
        return 0xFD;
    case 2:
        return 0xED;
    case 3:
        return 0x7D;
    case 4:
        return 0x6D;
    default:
        return 0xFF;
    }
}

quint8 BkuHardware::openFirstDoorCommand() const
{
    return 0xF7;
}

quint8 BkuHardware::closeFirstDoorCommand() const
{
    return 0xDF;
}

quint8 BkuHardware::openSecondDoorCommand() const
{
    return 0xFB;
}

quint8 BkuHardware::closeSecondDoorCommand() const
{
    return 0xF7;
}

bool BkuHardware::doOpen()
{
    auto cfg = currentConfiguration();
    m_enableMovingRack = currentConfiguration().value(RACK_MOVING).toBool();
    m_rack_move_timeout = currentConfiguration().value(RACK_MOVE_TIMEOUT).toInt();
    m_door_move_timeout = currentConfiguration().value(DOOR_MOVE_TIMEOUT).toInt();

    m_accessor = dispatcher().getAccessor(0x25);
    return true;
}

void BkuHardware::doClose()
{

}

bool BkuHardware::doOpenDoor(Door door, CancelationTokenSource::Token token)
{
    NpFrame openCommand;
    if (door == DoorFirst) {
        openCommand = NpFrame(0x25, 0x3D, openFirstDoorCommand(), 0xFB, 0xFF);
    } else if (door == DoorSecond) {
        // For moving second door rack must be in the top by design
        if (!doMoveRackToTop(token)) {
            return false;
        }

        openCommand = NpFrame(0x25, 0x3D, 0xFF, 0xFF, openSecondDoorCommand());
    } else {
        setLastError(tr("Неподдерживаемый индекс двери"));
        return false;
    }

    if (!m_accessor->write(openCommand)) {
        setLastError(tr("Не удалось отправить команду открытия двери"));
        return false;
    }

    bool opened = checkState([this, door] {
        return doorState(door) == DoorStateOpen;
    }, m_door_move_timeout, token);

    if (!clearMechanicsRegs()) {
        return false;
    }

    if (!opened) {
        setLastError(tr("Таймаут ожидания открытия двери"));
        return false;
    }

    return true;
}

bool BkuHardware::doCloseDoor(Door door, CancelationTokenSource::Token token)
{
    NpFrame closeCommand;

    if (door == DoorFirst) {
        closeCommand = NpFrame(0x25, 0x3D, closeFirstDoorCommand(), 0xFD, 0xFF);
    } else if (door == DoorSecond) {
        // For moving second door rack must be in the top by design
        if (!doMoveRackToTop(token)) {
            return false;
        }

        closeCommand = NpFrame(0x25, 0x3D, 0xFF, 0xFF, closeSecondDoorCommand());
    } else {
        setLastError(tr("Неподдерживаемый индекс двери"));
        return false;
    }

    if (!m_accessor->write(closeCommand)) {
        setLastError(tr("Не удалось отправить команду закрытия двери"));
        return false;
    }

    bool closed = checkState([this, door] {
        return doorState(door) == DoorStateClosed;
    }, m_door_move_timeout, token);

    if (!clearMechanicsRegs()) {
        return false;
    }

    if (!closed) {
        setLastError(tr("Таймаут ожидания закрытия двери"));
        return false;
    }

    return true;
}

bool BkuHardware::doMoveRackToBottom(CancelationTokenSource::Token token)
{
    if (!m_enableMovingRack) {
        return true;
    }

    if (!doMoveRackDown()) {
        return false;
    }

    bool bottom = checkState([this] {
        return rackState() == RackStateBottom;
    }, m_rack_move_timeout, token);

    if (!doMoveRackStop()) {
        return false;
    }

    if (!bottom) {
        setLastError(tr("Таймаут ожидания штатива"));
        return false;
    }

    return true;
}

bool BkuHardware::doMoveRackToTop(CancelationTokenSource::Token token)
{
    if (!m_enableMovingRack) {
        return true;
    }

    if (!doMoveRackUp()) {
        return false;
    }

    bool top = checkState([this] {
        return rackState() == RackStateTop;
    }, m_rack_move_timeout, token);

    if (!doMoveRackStop()) {
        return false;
    }

    if (!top) {
        setLastError(tr("Таймаут ожидания штатива"));
        return false;
    }

    return true;
}

bool BkuHardware::doLockRemote()
{
    NpFrame lockCommand(0x25, 0x7D);
    if (!m_accessor->write(lockCommand)) {
        setLastError(tr("Не удалось отправить команду блокировки пульта"));
        return false;
    }

    return true;
}

bool BkuHardware::doUnlockRemote()
{
    NpFrame unlockCommand(0x25, 0x6D);
    if (!m_accessor->write(unlockCommand)) {
        setLastError(tr("Не удалось отправить команду разблокировки пульта"));
        return false;
    }

    return true;
}

bool BkuHardware::doMoveRackToBottomAndOpenFirstDoor()
{
    if (!m_enableMovingRack) {
        return doOpenDoor(DoorFirst, CancelationTokenSource::Token());
    }

    const quint8 firstReg = openFirstDoorCommand() & speedRackScanDownCommand();
    NpFrame returnAndOpenCommand(0x25, 0x3D, firstReg, 0xEF, 0xFF);
    if (!m_accessor->write(returnAndOpenCommand)) {
        setLastError(tr("Не удалось отправить команду управления механикой"));
        return false;
    }

    bool bottomAndOpened = checkState([this] {
        return rackState() == RackStateBottom &&
               doorState(DoorFirst) == DoorStateOpen;
    }, qMax(m_door_move_timeout, m_rack_move_timeout));

    if (!clearMechanicsRegs()) {
        return false;
    }

    if (!bottomAndOpened) {
        setLastError(tr("Таймаут ожидания механики"));
        return false;
    }

    return true;
}

bool BkuHardware::doStartScan()
{
    if (!m_accessor->write(m_enableMovingRack ?
                           NpFrame(0x25, 0x1D, speedRackScanUpCommand(), 0xF7, 0x7F) :
                           NpFrame(0x25, 0x3D, 0xFF, 0xFF, 0x7F))) {
        setLastError(tr("Не удалось отправить команду старта"));
        return false;
    }

    return true;
}

bool BkuHardware::doStopScan()
{
    return doStop();
}

bool BkuHardware::doStop()
{
    if (!m_accessor->write(NpFrame(0x25, 0x2D))) {
        setLastError(tr("Не удалось отправить команду стопа"));
        return false;
    }

    return true;
}

bool BkuHardware::doMoveRackToBottomAndCloseFirstDoor()
{
    if (!m_enableMovingRack) {
        return doCloseDoor(DoorFirst, CancelationTokenSource::Token());
    }

    const quint8 firstReg = closeFirstDoorCommand() & speedRackScanDownCommand();
    NpFrame returnAndOpenCommand(0x25, 0x3D, firstReg, 0xEF, 0xFF);
    if (!m_accessor->write(returnAndOpenCommand)) {
        setLastError(tr("Не удалось отправить команду управления механикой"));
        return false;
    }

    bool bottomAndClosed = checkState([this] {
        return rackState() == RackStateBottom &&
               doorState(DoorFirst) == DoorStateClosed;
    }, qMax(m_door_move_timeout, m_rack_move_timeout));

    if (!clearMechanicsRegs()) {
        return false;
    }

    if (!bottomAndClosed) {
        setLastError(tr("Таймаут ожидания механики"));
        return false;
    }

    return true;
}

bool BkuHardware::doPressPrepareButton()
{
    NpFrame pressPrepareButtonCommand(0x25, 0xAD);
    if (m_accessor->write(pressPrepareButtonCommand)) {
        // Not documented delay
        QThread::msleep(1000);
        return true;
    }

    return false;
}

void BkuHardware::doRefreshState(QMap<Door, DoorState> &doors, RackState &rack)
{
    DoorState firstDoorState = DoorStateUnknown;
    DoorState secondDoorState = DoorStateUnknown;
    RackState rackState = RackStateUnknown;

    NpFrame getStateCommand(0x25, 0x4D, 0x00, 0x00, 0xFF);
    NpFrame currentState = m_accessor->writeAndRead(getStateCommand);
    if (currentState.isValid() && currentState.cmd() == 0x5D) {
        {
            bool firstDoorIsClosed = ((currentState.reg1() & (1 << 2)) &&
                                     !(currentState.reg1() & (1 << 3)));
            bool firstDoorIsOpen = (!(currentState.reg1() & (1 << 2)) &&
                                   (currentState.reg1() & (1 << 3)));

            if (firstDoorIsOpen && !firstDoorIsClosed) {
                firstDoorState = DoorStateOpen;
            } else if (!firstDoorIsOpen && firstDoorIsClosed) {
                firstDoorState = DoorStateClosed;
            }
        }

        {
            bool secondDoorIsClosed = (currentState.reg1() & 1) &&
                                      !(currentState.reg1() & (1 << 1));

            bool secondDoorIsOpen = !(currentState.reg1() & 1) &&
                                    (currentState.reg1() & (1 << 1));

            if (secondDoorIsOpen && !secondDoorIsClosed) {
                secondDoorState = DoorStateOpen;
            } else if (!secondDoorIsOpen && secondDoorIsClosed) {
                secondDoorState = DoorStateClosed;
            }
        }

        {
            bool rackAtBottom = !(currentState.reg1() & (1 << 7)) &&
                                (currentState.reg1() & (1 << 6));

            bool rackAtTop = (currentState.reg1() & (1 << 7)) &&
                             !(currentState.reg1() & (1 << 6));

            if (rackAtTop && !rackAtBottom) {
                rackState = RackStateTop;
            } else if (!rackAtTop && rackAtBottom) {
                rackState = RackStateBottom;
            }
        }
    }

    doors.insert(DoorFirst, firstDoorState);
    doors.insert(DoorSecond, secondDoorState);
    rack = rackState;
}

DeviceConfiguration BkuHardware::defaultConfiguration() const
{
    DeviceConfiguration conf;
    conf.insert(RACK_MOVING, true, tr("Движение механики"));
    conf.insert(RACK_UP_SPEED, 3, tr("Ячейка скорости при движении вверх [1-4]"));
    conf.insert(RACK_DOWN_SPEED, 3, tr("Ячейка скорости при движении вниз [1-4]"));
    conf.insert(RACK_MOVE_TIMEOUT, 10000, tr("Таймаут движения штатива, мс [>0]"));
    conf.insert(DOOR_MOVE_TIMEOUT, 7000, tr("Таймаут движения двери, мс [>0]"));
    return conf;
}


bool BkuHardware::doMoveRackUp()
{
    if (!m_enableMovingRack) {
        return true;
    }

    NpFrame moveUpCommand(0x25, 0x3D, speedRackScanUpCommand(), 0xEF, 0xFD);
    if (!m_accessor->write(moveUpCommand)) {
        setLastError(tr("Не удалось отправить команду движения штатива вверх"));
        return false;
    }

    return true;
}

bool BkuHardware::doMoveRackDown()
{
    if (!m_enableMovingRack) {
        return true;
    }

    NpFrame moveDownCommand(0x25, 0x3D, speedRackScanDownCommand(), 0xEF, 0xFD);
    if (!m_accessor->write(moveDownCommand)) {
        setLastError(tr("Не удалось отправить команду движения штатива вниз"));
        return false;
    }

    return true;
}

bool BkuHardware::doMoveRackStop()
{
    if (!m_enableMovingRack) {
        return true;
    }

    return clearMechanicsRegs();
}
