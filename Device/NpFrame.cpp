#include "NpFrame.h"

#include "DeviceLogging.h"

NpFrame::NpFrame()
{

}

NpFrame::NpFrame(quint8 addr, quint8 cmd,
                 quint8 reg1, quint8 reg2, quint8 reg3)
{
    m_data = QByteArray(size, 0);
    m_data[0] = addr;
    m_data[1] = cmd;
    m_data[2] = reg1;
    m_data[3] = reg2;
    m_data[4] = reg3;
    m_data[5] = calcChecksum();
}

NpFrame::NpFrame(const QByteArray& bytes):
    m_data(bytes)
{

}

quint8 NpFrame::addr() const
{
    return getByte(0);
}

quint8 NpFrame::cmd() const
{
    return getByte(1);
}

quint8 NpFrame::reg1() const
{
    return getByte(2);
}

quint8 NpFrame::reg2() const
{
    return getByte(3);
}

quint8 NpFrame::reg3() const
{
    return getByte(4);
}

QByteArray NpFrame::toByteArray() const
{
    return m_data;
}

QByteArray NpFrame::toHex() const
{
    return m_data.toHex();
}

NpFrame::operator QByteArray() const
{
    return m_data;
}

bool NpFrame::isValid() const
{
    const auto length = m_data.length();
    if (length != size) {
        errDevice << "Frame:" << m_data.toHex()
                  << "Length:" << length
                  << "Expected length:" << size;
        return false;
    }

    auto receivedChecksum = static_cast<quint8>(m_data.at(5));
    auto expectedChecksum = calcChecksum();

    if (receivedChecksum != expectedChecksum) {
        errDevice << "Frame:" << m_data.toHex()
                  << "Checksum:" << QString::number(receivedChecksum, 16)
                  << "Expected checksum:" << QString::number(expectedChecksum, 16);
        return false;
    }

    return true;
}

QVector<NpFrame> NpFrame::splitFrames(const QByteArray &bytes)
{
    QVector<NpFrame> frames;

    for (int i = 0; i < bytes.length(); i += size) {
        frames.append(bytes.mid(i, size));
    }

    return frames;
}

quint8 NpFrame::calcChecksum() const
{
    quint8 sum = m_data.at(0);
    for (int i = 1; i < m_data.length() - 1; ++i) {
        sum ^= m_data.at(i);
    }

    return sum;
}

quint8 NpFrame::getByte(int index) const
{
    return m_data.size() > index ? m_data.at(index) : 0;
}
