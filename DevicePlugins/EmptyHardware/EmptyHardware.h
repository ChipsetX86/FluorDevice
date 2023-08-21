#ifndef EMPTYHARDWARE_H
#define EMPTYHARDWARE_H

#include <Device/Hardware.h>

class EmptyHardware : public Hardware
{
    Q_OBJECT
    Q_DISABLE_COPY(EmptyHardware)
public:
    explicit EmptyHardware(QObject *parent = nullptr);
    ~EmptyHardware() override;
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
    QMap<Hardware::Door, Hardware::DoorState> m_doors;
    Hardware::RackState m_rack;
};

#endif // EMPTYHARDWARE_H
