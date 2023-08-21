#ifndef BKUHARDWARE_H
#define BKUHARDWARE_H

#include <Device/Hardware.h>
#include <Device/Dispatcher.h>

#include <functional>

class BkuHardware : public Hardware
{
    Q_OBJECT
    Q_DISABLE_COPY(BkuHardware)
public:
    explicit BkuHardware(QObject *parent = nullptr);
    ~BkuHardware() override;
    DeviceConfiguration defaultConfiguration() const override;
protected:
    bool doOpen() override;
    void doClose() override;
    bool doTestConnection() override;
    bool doOpenDoor(Door door, CancelationTokenSource::Token token) override;
    bool doCloseDoor(Door door, CancelationTokenSource::Token token) override;
    bool doMoveRackToBottom(CancelationTokenSource::Token token) override;
    bool doMoveRackToBottomAndOpenFirstDoor() override;
    bool doMoveRackToBottomAndCloseFirstDoor() override;
    bool doMoveRackToTop(CancelationTokenSource::Token token) override;
    bool doMoveRackUp() override;
    bool doMoveRackDown() override;
    bool doMoveRackStop() override;
    bool doLockRemote() override;
    bool doUnlockRemote() override;
    bool doPressPrepareButton() override;
    bool doStartScan() override;
    bool doStopScan() override;
    bool doStop() override;
    void doRefreshState(QMap<Door, DoorState> &doors, RackState &rack) override;
private:
    typedef std::function<bool ()> CheckStateCallback;
    bool checkState(const CheckStateCallback &callback, int timeout, CancelationTokenSource::Token token = CancelationTokenSource::Token());
    bool clearMechanicsRegs();
    quint8 speedRackScanDownCommand() const;
    quint8 speedRackScanUpCommand() const;
    quint8 openFirstDoorCommand() const;
    quint8 closeFirstDoorCommand() const;
    quint8 openSecondDoorCommand() const;
    quint8 closeSecondDoorCommand() const;

    Dispatcher::Accessor *m_accessor;
    bool m_enableMovingRack;
    ushort m_rack_move_timeout;
    ushort m_door_move_timeout;
};

#endif // BKUHARDWARE_H
