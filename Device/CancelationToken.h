#ifndef CANCELATIONTOKEN_H
#define CANCELATIONTOKEN_H

#include "DeviceGlobal.h"

#include <QSharedPointer>

class DEVICELIB_EXPORT CancelationTokenSource
{
public:
    class DEVICELIB_EXPORT Token
    {
    public:
        Token();
        bool isCanceled() const;
    private:
        friend class CancelationTokenSource;
        void cancel();

        struct PImpl;
        QSharedPointer<PImpl> m_pimpl;
    };

    CancelationTokenSource();
    void cancel();

    Token token() const;
private:
    Token m_token;
};

#endif // CANCELATIONTOKEN_H
