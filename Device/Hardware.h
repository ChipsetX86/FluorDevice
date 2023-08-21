#ifndef HARDWARE_H
#define HARDWARE_H

#include "Device.h"

#include <QMap>
#include <QScopedPointer>

#include "CancelationToken.h"

class PowerSupply;

class DEVICELIB_EXPORT Hardware : public Device
{
    Q_OBJECT
    Q_DISABLE_COPY(Hardware)
public:
    enum Door {
        DoorFirst,
        DoorSecond
    };

    enum DoorState {
        DoorStateUnknown,
        DoorStateOpen,
        DoorStateClosed
    };

    enum RackState {
        RackStateUnknown,
        RackStateBottom,
        RackStateTop
    };

    explicit Hardware(QObject *parent = nullptr);
    ~Hardware();

    DoorState doorState(Door door);
    RackState rackState();
public slots:
    bool openDoor(Hardware::Door door);
    bool openDoor(Hardware::Door door, CancelationTokenSource::Token token);    

    bool closeDoor(Hardware::Door door);
    bool closeDoor(Hardware::Door door, CancelationTokenSource::Token token);

    bool openFirstDoor();
    bool openFirstDoor(CancelationTokenSource::Token token);

    bool closeFirstDoor();
    bool closeFirstDoor(CancelationTokenSource::Token token);

    bool moveRackToBottom();
    bool moveRackToBottom(CancelationTokenSource::Token token);

    bool moveRackToBottomAndOpenFirstDoor();
    bool moveRackToBottomAndCloseFirstDoor();

    bool moveRackToTop();
    bool moveRackToTop(CancelationTokenSource::Token token);

    bool moveRackUp();
    bool moveRackDown();
    bool moveRackStop();

    bool lockRemote();
    bool unlockRemote();

    bool refreshState();

    bool pressPrepareButton();
    bool startScan(PowerSupply *powerSupply);
    bool stopScan();
    bool stop();
protected:
    virtual bool doOpenDoor(Door door, CancelationTokenSource::Token token) = 0;
    virtual bool doCloseDoor(Door door, CancelationTokenSource::Token token) = 0;
    virtual bool doMoveRackToBottom(CancelationTokenSource::Token token) = 0;
    virtual bool doMoveRackToBottomAndOpenFirstDoor() = 0;
    virtual bool doMoveRackToBottomAndCloseFirstDoor() = 0;
    virtual bool doMoveRackToTop(CancelationTokenSource::Token token) = 0;
    virtual bool doMoveRackUp() = 0;
    virtual bool doMoveRackDown() = 0;
    virtual bool doMoveRackStop() = 0;
    virtual bool doLockRemote() = 0;
    virtual bool doUnlockRemote() = 0;
    virtual void doRefreshState(QMap<Door, DoorState> &doors, RackState &rack) = 0;
    virtual bool doPressPrepareButton() = 0;
    virtual bool doStartScan() = 0;
    virtual bool doStopScan() = 0;
    virtual bool doStop() = 0;
private slots:
    void resetState();
private:
    void onClosed() override final;

    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif // HARDWARE_H
