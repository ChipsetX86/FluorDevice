#include "Device.h"

#include <QMutex>
#include <QMutexLocker>
#include <QThread>

#include <NpToolbox/Toolbox.h>

#include "Dispatcher.h"
#include "DeviceLogging.h"

using namespace Nauchpribor;

struct Device::PImpl
{
    DeviceConfiguration currentConfiguration;

    Dispatcher *dispatcher;

    QMutex lastErrorMutex;
    QString lastError;

    bool isOpen;
};

Device::Device(QObject *parent) : QObject(parent),
    m_pimpl(new PImpl)
{
    m_pimpl->isOpen = false;
    m_pimpl->dispatcher = nullptr;
}

Device::~Device()
{

}

QString Device::lastError() const
{
    QMutexLocker locker(&m_pimpl->lastErrorMutex);
    return m_pimpl->lastError;
}

void Device::setDispatcher(Dispatcher *dispatcher)
{
    infoDevice << "Set dispatcher for device";

    m_pimpl->dispatcher = dispatcher;
}

DeviceConfiguration Device::defaultConfiguration() const
{
    return DeviceConfiguration();
}

QString Device::name() const
{
    return Toolbox::fromCamelCase(metaObject()->className());
}

bool Device::open(const DeviceConfiguration &configuration)
{
    infoDevice << "Trying to open device";

    Q_ASSERT(checkThreadAffinity());

    if (m_pimpl->isOpen) {
        errDevice << "Device is already opened";
        setLastError(tr("Устройство уже открыто"));
        return false;
    }

    m_pimpl->currentConfiguration = defaultConfiguration();
    m_pimpl->currentConfiguration.replaceValues(configuration);

    if ((m_pimpl->isOpen = doOpen())) {
        infoDevice << "Device successfully opened";
        onOpened();
    } else {
        errDevice << "Failed to open device";
    }

    return m_pimpl->isOpen;
}

bool Device::reOpen(const DeviceConfiguration &configuration)
{
    infoDevice << "Trying to reopen device";

    Q_ASSERT(checkThreadAffinity());

    DeviceConfiguration newConfiguration = defaultConfiguration();
    newConfiguration.replaceValues(configuration);

    if (m_pimpl->isOpen) {
        if (newConfiguration == m_pimpl->currentConfiguration) {
            infoDevice << "Device configurations matching. Do nothing";
            return true;
        }

        infoDevice << "Device configurations mismatch. Closing device";

        doClose();
        m_pimpl->isOpen = false;
        onClosed();
    }

    m_pimpl->currentConfiguration = newConfiguration;

    if ((m_pimpl->isOpen = doOpen())) {
        infoDevice << "Device successfully reopened";
        onOpened();
    } else {
        errDevice << "Failed to reopen device";
    }

    return m_pimpl->isOpen;
}

bool Device::reset()
{
    infoDevice << "Trying to reset device";

    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen() || !doReset()) {
        errDevice << "Failed to reset device";
        return false;
    }

    emit reseted(QPrivateSignal());
    infoDevice << "Device successfully reseted";
    return true;
}

bool Device::isOpen()
{
    Q_ASSERT(checkThreadAffinity());

    return m_pimpl->isOpen;
}

void Device::close()
{
    infoDevice << "Trying to close device";

    Q_ASSERT(checkThreadAffinity());

    if (!m_pimpl->isOpen) {
        warnDevice << "Device is not open yet";
        return;
    }

    doClose();
    m_pimpl->isOpen = false;
    onClosed();

    infoDevice << "Device successfully closed";
}

bool Device::testConnection()
{
    Q_ASSERT(checkThreadAffinity());

    return checkIsOpen() && doTestConnection();
}

void Device::setLastError(const QString &error)
{
    QMutexLocker locker(&m_pimpl->lastErrorMutex);
    m_pimpl->lastError = error;
    emit errorOccurred(m_pimpl->lastError, QPrivateSignal());
}

DeviceConfiguration &Device::currentConfiguration() const
{
    return m_pimpl->currentConfiguration;
}

bool Device::checkThreadAffinity() const
{
    return thread() == QThread::currentThread();
}

Dispatcher &Device::dispatcher() const
{
    Q_ASSERT(m_pimpl->dispatcher);
    
    return *(m_pimpl->dispatcher);
}

void Device::onOpened()
{
    emit opened(QPrivateSignal());
}

void Device::onClosed()
{
    emit closed(QPrivateSignal());
}

bool Device::checkIsOpen()
{
    if (!m_pimpl->isOpen) {
        errDevice << "Device is not opened yet";
        setLastError(tr("Устройство не открыто"));
        return false;
    }

    return true;
}

bool Device::doReset()
{
    return true;
}
