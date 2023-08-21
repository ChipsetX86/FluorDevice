#include "Detector.h"

#include <QMutexLocker>

#include <NpToolbox/Atomic.h>

using namespace Nauchpribor;

struct Detector::PImpl
{
    quint32 linesCount;
    bool isPrepared;
    Toolbox::Atomic<Properties> properties;
    Toolbox::Atomic<Frame> lastCapturedFrame;
};

Detector::Detector(QObject *parent) : Device(parent),
    m_pimpl(new PImpl)
{
    m_pimpl->linesCount = 0;
    m_pimpl->isPrepared = false;
}

Detector::~Detector()
{

}

Detector::Properties Detector::properties() const
{
    return m_pimpl->properties;
}

Detector::Frame Detector::lastCapturedFrame() const
{
    return m_pimpl->lastCapturedFrame;
}

bool Detector::prepare(quint32 lines)
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    if (lines < 1) {
        setLastError(tr("Неверное кол-во строк"));
        return false;
    }

    m_pimpl->linesCount = lines;

    if ((m_pimpl->isPrepared = doPrepare())) {
        emit prepared(QPrivateSignal());
    }

    return m_pimpl->isPrepared;
}

bool Detector::capture()
{
    Q_ASSERT(checkThreadAffinity());

    if (!checkIsOpen()) {
        return false;
    }

    if (!m_pimpl->isPrepared) {
        setLastError(tr("Детектор не готов"));
        return false;
    }

    m_pimpl->isPrepared = false;

    if (doCapture()) {
        emit captured(m_pimpl->lastCapturedFrame, QPrivateSignal());
        return true;
    }

    return false;
}

quint32 Detector::currentLines() const
{
    return m_pimpl->linesCount;
}

void Detector::setProperties(const Detector::Properties &properties)
{
    m_pimpl->properties = properties;
}

void Detector::setLastCapturedFrame(const Frame &frame)
{
    m_pimpl->lastCapturedFrame = frame;
}

void Detector::onClosed()
{
    m_pimpl->isPrepared = false;
    Device::onClosed();
}

Detector::Properties::Properties() :
    width(0),
    pixelSizeMm(0, 0),
    chargeTimeMsec(0)
{

}
