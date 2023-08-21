#include "PowerSupply.h"

#include <QWaitCondition>
#include <QMutex>
#include <QElapsedTimer>
#include <QThread>

struct PowerSupply::PImpl
{
    bool isOn;
    bool isPrepared;
    Params params;
    QMutex resultsMutex;
    Results results;

    QMutex buttonsMutex;
    QWaitCondition buttonsReleasedWaitCondition;
    bool isButtonsReleased;
    QElapsedTimer lastRunTimer;
    qint64 lastRunMs;
};

PowerSupply::PowerSupply(QObject *parent) : Device(parent),
    m_pimpl(new PImpl)
{
    m_pimpl->isOn = false;
    m_pimpl->isPrepared = false;
    m_pimpl->isButtonsReleased = false;
    m_pimpl->lastRunMs = 0;
}

PowerSupply::~PowerSupply() 
{

}

PowerSupply::Results PowerSupply::results() const
{
    QMutexLocker lock(&m_pimpl->resultsMutex);
    return m_pimpl->results;
}

void PowerSupply::buttonsReleased()
{
    QMutexLocker lock(&m_pimpl->buttonsMutex);
    m_pimpl->isButtonsReleased = true;
    m_pimpl->buttonsReleasedWaitCondition.wakeAll();
}

bool PowerSupply::prepare(const PowerSupply::Params &params)
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    if (m_pimpl->isOn) {
        setLastError(tr("РПУ уже включено"));
        return false;
    }

    if (params.voltageKV < 0.01) {
        setLastError(tr("Неверное значение напряжения"));
        return false;
    }

    if (params.amperageMA < 0.01) {
        setLastError(tr("Неверное значение тока"));
    }

    if (params.exposureMs < 1) {
        setLastError(tr("Неверное время экспозиции"));
        return false;
    }

    m_pimpl->params = params;

    if ((m_pimpl->isPrepared = doPrepare())) {
        emit prepared(QPrivateSignal());
    }

    return m_pimpl->isPrepared;
}

bool PowerSupply::launch()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    if (m_pimpl->isOn) {
        setLastError(tr("РПУ уже включено"));
        return false;
    }

    if (!m_pimpl->isPrepared) {
        setLastError(tr("РПУ не готово"));
        return false;
    }

    m_pimpl->isPrepared = false;

    {
        QMutexLocker lock(&m_pimpl->buttonsMutex);
        m_pimpl->isButtonsReleased = false;
    }

    if (!doOn()) {
        return false;
    }

    m_pimpl->isOn = true;
    m_pimpl->lastRunTimer.start();

    emit toggled(true, QPrivateSignal());

    const bool errorOccured = doWaitForError();

    if (!doOff()) {
        return false;
    }

    m_pimpl->isOn = false;
    m_pimpl->lastRunMs = m_pimpl->lastRunTimer.elapsed();

    emit toggled(false, QPrivateSignal());
    return !errorOccured;
}

bool PowerSupply::getResults()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    if (m_pimpl->isOn) {
        setLastError(tr("РПУ еще включено"));
        return false;
    }

    Results results;

    if (!doGetResults(results)) {
        return false;
    }

    if (!results.exposureMs) {
        results.exposureMs = m_pimpl->lastRunMs;
    }

    QMutexLocker lock(&m_pimpl->resultsMutex);
    m_pimpl->results = results;
    return true;
}

const PowerSupply::Params &PowerSupply::currentParams() const
{
    return m_pimpl->params;
}

bool PowerSupply::waitForButtonsReleased()
{
    QMutexLocker lock(&m_pimpl->buttonsMutex);

    if (m_pimpl->isButtonsReleased) {
        return true;
    }

    return m_pimpl->buttonsReleasedWaitCondition.wait(&m_pimpl->buttonsMutex);
}

bool PowerSupply::doWaitForError()
{
    QThread::msleep(currentParams().exposureMs);
    return false;
}

void PowerSupply::onClosed()
{
    m_pimpl->isOn = false;
    m_pimpl->isPrepared = false;
    m_pimpl->isButtonsReleased = false;
    Device::onClosed();
}

PowerSupply::Params::Params() :
    exposureMs(0),
    amperageMA(0),
    voltageKV(0)
{

}

PowerSupply::Results::Results() :
    exposureMs(0),
    amperageMA(0),
    voltageKV(0)
{

}
