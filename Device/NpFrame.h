#ifndef NPFRAME_H
#define NPFRAME_H

#include <QByteArray>
#include <QVector>

#include "DeviceGlobal.h"

class DEVICELIB_EXPORT NpFrame final
{
public:
    NpFrame();
    NpFrame(quint8 addr, quint8 cmd, quint8 reg1 = 0, quint8 reg2 = 0, quint8 reg3 = 0);
    NpFrame(const QByteArray &bytes);

    quint8 addr() const;
    quint8 cmd() const;
    quint8 reg1() const;
    quint8 reg2() const;
    quint8 reg3() const;

    QByteArray toByteArray() const;
    QByteArray toHex() const;
    operator QByteArray() const;
    bool isValid() const;
    static QVector<NpFrame> splitFrames(const QByteArray&);
    static constexpr int size = 6;
private:
    quint8 calcChecksum() const;
    quint8 getByte(int index) const;
    QByteArray m_data;
};

#endif // NPFRAME_H
