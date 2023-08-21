#include "Hardware.h"

#include <QThread>
#include <QMutex>

#include "PowerSupply.h"

struct Hardware::PImpl
{
    bool isScanStarted;
    PowerSupply *powerSupply;

    QMutex stateMutex;
    QMap<Door, DoorState> doorsState;
    RackState rackState;
};

Hardware::Hardware(QObject *parent) : Device(parent),
    m_pimpl(new PImpl)
{
    m_pimpl->isScanStarted = false;
    m_pimpl->powerSupply = nullptr;

    resetState();
}

Hardware::~Hardware()
{

}

Hardware::DoorState Hardware::doorState(Hardware::Door door)
{
    QMutexLocker lock(&m_pimpl->stateMutex);
    return m_pimpl->doorsState.value(door, DoorStateUnknown);
}

Hardware::RackState Hardware::rackState()
{
    QMutexLocker lock(&m_pimpl->stateMutex);
    return m_pimpl->rackState;
}

bool Hardware::openDoor(Hardware::Door door)
{
    return openDoor(door, CancelationTokenSource::Token());
}

bool Hardware::openDoor(Hardware::Door door, CancelationTokenSource::Token token)
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doOpenDoor(door, token);
}

bool Hardware::closeDoor(Hardware::Door door)
{
    return closeDoor(door, CancelationTokenSource::Token());
}

bool Hardware::closeDoor(Hardware::Door door, CancelationTokenSource::Token token)
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doCloseDoor(door, token);
}

bool Hardware::openFirstDoor()
{
    return openFirstDoor(CancelationTokenSource::Token());
}

bool Hardware::openFirstDoor(CancelationTokenSource::Token token)
{
    return openDoor(Door::DoorFirst, token);
}

bool Hardware::closeFirstDoor()
{
    return closeFirstDoor(CancelationTokenSource::Token());
}

bool Hardware::closeFirstDoor(CancelationTokenSource::Token token)
{
    return closeDoor(Door::DoorFirst, token);
}

bool Hardware::moveRackToBottom()
{
    return moveRackToBottom(CancelationTokenSource::Token());
}

bool Hardware::moveRackToBottom(CancelationTokenSource::Token token)
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doMoveRackToBottom(token);
}

bool Hardware::moveRackToBottomAndOpenFirstDoor()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doMoveRackToBottomAndOpenFirstDoor();
}

bool Hardware::moveRackToBottomAndCloseFirstDoor()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doMoveRackToBottomAndCloseFirstDoor();
}

bool Hardware::moveRackToTop()
{
    return moveRackToTop(CancelationTokenSource::Token());
}

bool Hardware::moveRackToTop(CancelationTokenSource::Token token)
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doMoveRackToTop(token);
}

bool Hardware::moveRackUp()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doMoveRackUp();
}

bool Hardware::moveRackDown()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doMoveRackDown();
}

bool Hardware::moveRackStop()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doMoveRackStop();
}

bool Hardware::lockRemote()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doLockRemote();
}

bool Hardware::unlockRemote()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doUnlockRemote();
}

bool Hardware::refreshState()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    QMap<Door, DoorState> doors;
    RackState rack = RackStateUnknown;

    doRefreshState(doors, rack);

    QMutexLocker lock(&m_pimpl->stateMutex);
    m_pimpl->doorsState = doors;
    m_pimpl->rackState = rack;

    return true;
}

bool Hardware::pressPrepareButton()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doPressPrepareButton();
}

bool Hardware::startScan(PowerSupply *powerSupply)
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    if ((m_pimpl->isScanStarted = doStartScan())) {
        m_pimpl->powerSupply = powerSupply;
    }

    return m_pimpl->isScanStarted;
}

bool Hardware::stopScan()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    if (!m_pimpl->isScanStarted) {
        setLastError(tr("Сканирование не запущено"));
        return false;
    }

    if (doStopScan()) {
        if (m_pimpl->powerSupply) {
            m_pimpl->powerSupply->buttonsReleased();
        }

        m_pimpl->isScanStarted = false;
        return true;
    }

    return false;
}

bool Hardware::stop()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    return doStop();
}

void Hardware::resetState()
{
    QMutexLocker lock(&m_pimpl->stateMutex);
    m_pimpl->doorsState.clear();
    m_pimpl->rackState = RackStateUnknown;
}

void Hardware::onClosed()
{
    resetState();
    Device::onClosed();
}
