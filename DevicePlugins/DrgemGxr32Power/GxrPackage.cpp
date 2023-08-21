#include "GxrPackage.h"

#include <Device/DeviceLogging.h>

GxrPackage::GxrPackage():
    m_data{0}
{
    m_data[Pos::Header1] = UDP_PACKET_HEADER1;
    m_data[Pos::Header2] = UDP_PACKET_HEADER2;
    m_data[Pos::Address] = UDP_PACKET_FROM_CLIENT;
    m_data[Pos::Tail] = UDP_PACKET_TAIL;
}

GxrPackage::GxrPackage(GxrRegister gxrCommand):
    GxrPackage()
{
    m_data[Pos::Command] = gxrCommand;
    updateChecksum();
}

GxrPackage::GxrPackage(const QByteArray &data):
    GxrPackage()
{
    fromQByteArray(data);
}

GxrRegister GxrPackage::payload(int index) const
{
    if (index < 0 || index >= m_sizePayload) {
        errDevice << "Index out of range in GXR payload method payload";
        return 0;
    }

    return m_data[Pos::StartData + index];
}

void GxrPackage::setPayload(int index, const GxrRegister value)
{
    if (index < 0 || index >= m_sizePayload) {
        errDevice << "Index out of range in GXR payload method setPayload";
    } else {
        m_data[Pos::StartData + index] = value;
        updateChecksum();
    }
}

GxrRegister GxrPackage::command() const
{
    return m_data[Pos::Command];
}

void GxrPackage::setCommand(GxrRegister command)
{
    m_data[Pos::Command] = command;
    updateChecksum();
}

GxrRegister GxrPackage::calcChecksum() const
{
    GxrRegister checkSum = m_data[Pos::Address] ^ m_data[Pos::Command];

    for (int i = 0; i < m_sizePayload; ++i) {
        checkSum ^= m_data[Pos::StartData + i];
    }

    return checkSum;
}

void GxrPackage::updateChecksum()
{
    m_data[Pos::Checksum] = calcChecksum();
}

bool GxrPackage::isValid() const
{
    return m_data[Pos::Header1] == UDP_PACKET_HEADER1 &&
           m_data[Pos::Header2] == UDP_PACKET_HEADER2 &&
           m_data[Pos::Tail] == UDP_PACKET_TAIL &&
           (m_data[Pos::Address] == UDP_PACKET_FROM_CLIENT || m_data[Pos::Address] == UDP_PACKET_FROM_SERVER) &&
           m_data[Pos::Checksum] == calcChecksum();
}

QByteArray GxrPackage::toQByteArray() const
{
    return QByteArray(reinterpret_cast<const char*>(&m_data), sizeof(m_data) / sizeof(char));
}

void GxrPackage::fromQByteArray(const QByteArray &data)
{
    if (data.size() == sizeof(m_data) / sizeof(char)) {
        memcpy(&m_data, data.constData(), sizeof(m_data));
    }
}
