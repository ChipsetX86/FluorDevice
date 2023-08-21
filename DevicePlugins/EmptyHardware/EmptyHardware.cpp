#include "EmptyHardware.h"

#include <QThread>
#include <QElapsedTimer>

#include <Device/DeviceLogging.h>

const uint DOOR_MOVE_DURATION_MS = 3000;
const uint RACK_MOVE_DURATION_MS = 6000;
const uint THREAD_SLEEP_MS = 100;

EmptyHardware::EmptyHardware(QObject *parent) : Hardware(parent)
{

}

EmptyHardware::~EmptyHardware()
{

}

bool EmptyHardware::doTestConnection()
{
    return true;
}

bool EmptyHardware::doOpen()
{
    return true;
}

void EmptyHardware::doClose()
{

}

bool EmptyHardware::doOpenDoor(Door door, CancelationTokenSource::Token token)
{
    dbgDevice << "Opening door ...";

    QElapsedTimer timer;
    timer.start();

    do {
        QThread::msleep(THREAD_SLEEP_MS);

        if (token.isCanceled()) {
            dbgDevice << "Opening door has been canceled";
            m_doors.insert(door, DoorStateUnknown);
            return false;
        }
    } while (!timer.hasExpired(DOOR_MOVE_DURATION_MS));

    dbgDevice << "Door is opened";
    m_doors.insert(door, DoorStateOpen);
    return true;
}

bool EmptyHardware::doCloseDoor(Door door, CancelationTokenSource::Token token)
{
    dbgDevice << "Closing door ...";

    QElapsedTimer timer;
    timer.start();

    do {
        QThread::msleep(THREAD_SLEEP_MS);

        if (token.isCanceled()) {
            dbgDevice << "Closing door has been canceled";
            m_doors.insert(door, DoorStateUnknown);
            return false;
        }
    } while (!timer.hasExpired(DOOR_MOVE_DURATION_MS));

    dbgDevice << "Door is closed";
    m_doors.insert(door, DoorStateClosed);
    return true;
}

bool EmptyHardware::doMoveRackToBottom(CancelationTokenSource::Token token)
{
    dbgDevice << "Moving rack to bottom ...";

    QElapsedTimer timer;
    timer.start();

    do {
        QThread::msleep(THREAD_SLEEP_MS);

        if (token.isCanceled()) {
            dbgDevice << "Rack moving has been canceled";
            m_rack = RackStateUnknown;
            return false;
        }

    } while (!timer.hasExpired(RACK_MOVE_DURATION_MS));

    dbgDevice << "Rack at bottom";
    m_rack = RackStateBottom;
    return true;
}

bool EmptyHardware::doMoveRackToBottomAndOpenFirstDoor()
{
    auto delay = qMax(DOOR_MOVE_DURATION_MS, RACK_MOVE_DURATION_MS);
    dbgDevice << "Sleep, ms:" << delay;
    QThread::msleep(delay);
    m_doors.insert(DoorFirst, DoorStateOpen);
    m_rack = RackStateBottom;
    return true;
}

bool EmptyHardware::doStartScan()
{
    m_rack = RackStateUnknown;
    return true;
}

bool EmptyHardware::doStop()
{
    m_rack = RackStateUnknown;
    return true;
}

bool EmptyHardware::doStopScan()
{
    m_rack = RackStateUnknown;
    return true;
}

bool EmptyHardware::doMoveRackToBottomAndCloseFirstDoor()
{
    auto delay = qMax(DOOR_MOVE_DURATION_MS, RACK_MOVE_DURATION_MS);
    dbgDevice << "Sleep, ms:" << delay;
    QThread::msleep(delay);
    m_doors.insert(DoorFirst, DoorStateClosed);
    m_rack = RackStateBottom;
    return true;
}

bool EmptyHardware::doPressPrepareButton()
{
    return true;
}

void EmptyHardware::doRefreshState(QMap<Hardware::Door, Hardware::DoorState> &doors,
                                   Hardware::RackState &rack)
{
    doors = m_doors;
    rack  = m_rack;
}

bool EmptyHardware::doMoveRackToTop(CancelationTokenSource::Token token)
{
    dbgDevice << "Moving rack to top ...";

    QElapsedTimer timer;
    timer.start();

    do {
        QThread::msleep(THREAD_SLEEP_MS);

        if (token.isCanceled()) {
            dbgDevice << "Rack moving has been canceled";
            m_rack = RackStateUnknown;
            return false;
        }
    } while (!timer.hasExpired(RACK_MOVE_DURATION_MS));

    dbgDevice << "Rack at top";
    m_rack = RackStateTop;
    return true;
}

bool EmptyHardware::doLockRemote()
{
    return true;
}

bool EmptyHardware::doUnlockRemote()
{
    return true;
}

bool EmptyHardware::doMoveRackUp()
{
    m_rack = RackStateUnknown;
    return true;
}

bool EmptyHardware::doMoveRackDown()
{
    m_rack = RackStateUnknown;
    return true;
}

bool EmptyHardware::doMoveRackStop()
{
    m_rack = RackStateUnknown;
    return true;
}
