#ifndef GXRPACKAGE_H
#define GXRPACKAGE_H

#include "Gxr.h"

class GxrPackage
{
public:
    GxrPackage();
    GxrPackage(GxrRegister gxrCommand);
    GxrPackage(const QByteArray& data);

    GxrRegister payload(int index) const;
    void setPayload(int index, const GxrRegister value);

    GxrRegister command() const;
    void setCommand(GxrRegister command);

    bool isValid() const;
    QByteArray toQByteArray() const;
    void fromQByteArray(const QByteArray &data);

    static constexpr int size = 14;
private:
    enum Pos {
      Header1 = 0,
      Header2 = 1,
      Address = 2,
      Command = 3,
      StartData = 4,
      Checksum = 12,
      Tail = 13
    };

    static constexpr int m_sizePayload = 8;

    GxrRegister m_data[size];

    GxrRegister calcChecksum() const;
    void updateChecksum();
};


#endif // GXRPACKAGE_H
