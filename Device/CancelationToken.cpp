#include "CancelationToken.h"

#include <QMutex>
#include <QMutexLocker>

struct CancelationTokenSource::Token::PImpl
{
    QMutex mutex;
    bool isCanceled;
};

CancelationTokenSource::CancelationTokenSource()
{

}

void CancelationTokenSource::cancel()
{
    m_token.cancel();
}

CancelationTokenSource::Token CancelationTokenSource::token() const
{
    return m_token;
}

bool CancelationTokenSource::Token::isCanceled() const
{
    QMutexLocker l(&m_pimpl->mutex);
    return m_pimpl->isCanceled;
}

void CancelationTokenSource::Token::cancel()
{
    QMutexLocker l(&m_pimpl->mutex);
    m_pimpl->isCanceled = true;
}

CancelationTokenSource::Token::Token() :
    m_pimpl(new PImpl)
{
    m_pimpl->isCanceled = false;
}
