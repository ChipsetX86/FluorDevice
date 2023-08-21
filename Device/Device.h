#ifndef DEVICE_H
#define DEVICE_H

#include "DeviceGlobal.h"

#include <QObject>
#include <QScopedPointer>

#include "DeviceConfiguration.h"

class Dispatcher;

class DEVICELIB_EXPORT Device : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(Device)
public:
    explicit Device(QObject *parent = nullptr);    
    virtual ~Device();

    QString lastError() const;
    void setDispatcher(Dispatcher *dispatcher);
    virtual DeviceConfiguration defaultConfiguration() const;
    virtual QString name() const;
public slots:
    bool open(const DeviceConfiguration &configuration);
    void close();
    bool reset();
    bool isOpen();
    bool testConnection();
    /**
     * If device is opened and configuration not changed 
     * this method do nothing and return true. Otherwise 
     * closing device if needed and opening with new 
     * configuration
     **/
    bool reOpen(const DeviceConfiguration &configuration);
signals:
    void opened(QPrivateSignal);
    void closed(QPrivateSignal);
    void errorOccurred(QString error, QPrivateSignal);
    void reseted(QPrivateSignal);
protected:
    bool checkIsOpen();
    virtual bool doOpen() = 0;
    virtual void doClose() = 0;
    virtual bool doTestConnection() = 0;
    /**
     * Default implementation do nothing
     **/
    virtual bool doReset();
    void setLastError(const QString &error);
    DeviceConfiguration &currentConfiguration() const;
    bool checkThreadAffinity() const;
    Dispatcher &dispatcher() const;

    virtual void onOpened();
    virtual void onClosed();
private:
    struct PImpl;
    QScopedPointer<PImpl> m_pimpl;
};

#endif // DEVICE_H
