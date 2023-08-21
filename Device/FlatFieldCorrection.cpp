#include "FlatFieldCorrection.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QtMath>

#include "DeviceLogging.h"

FlatFieldCorrection::FlatFieldCorrection(const QString &filename):
    m_filename(filename)
{
    loadGains();
}

QDateTime FlatFieldCorrection::lastCalibrationDateTime() const
{
    QMutexLocker locker(&m_lastCalibrationMutex);
    return m_lastCalibration;
}

int FlatFieldCorrection::width() const
{
    return m_gains.size();
}

QVector<float> FlatFieldCorrection::ratesFromDarkFrame(const QVector<float> &darkFrame, int width) const
{
    QVector<float> rates;
    rates.reserve(width);

    const int height = darkFrame.size() / width;
    for (int i = 0; i < width; ++i) {
        double sum = 0;
        for (int j = 0; j < height; ++j) {
            sum += static_cast<double>(darkFrame.at(width * j + i));
        }
        rates.append(static_cast<float>(sum / height));
    }

    return rates;
}

void FlatFieldCorrection::loadGains()
{
    infoDevice << "Loading gains from:" << m_filename;

    if (m_filename.isEmpty()) {
        errDevice << "Gains filename is empty";
        return;
    }

    QFile f(m_filename);
    if (!f.open(QIODevice::ReadOnly)) {
        errDevice << "Failed to open gains file with error:" << f.errorString();
        return;
    }

    QVector<float> gains;
    int width = 0;

    QTextStream s(&f);
    if (!s.atEnd()) {
        width = s.readLine().toInt();
    }

    while (!s.atEnd()) {
        gains.append(s.readLine().toFloat());
    }

    if (gains.isEmpty()) {
        errDevice << "Gains is empty";
        return;
    }

    if (gains.size() != width) {
        errDevice << "Gains file is corrupted";
        return;
    }

    m_gains = gains;

    QFileInfo fi(f);
    setLastCalibrationDateTime(fi.lastModified());

    infoDevice << "Gains successfully loaded. Image width is" << m_gains.size();
}

bool FlatFieldCorrection::saveGains(const QVector<float> &gains)
{
    if (m_filename.isEmpty()) {
        errDevice << "Gains filename is empty";
        return false;
    }
    QFile f(m_filename);
    if (!f.open(QFile::WriteOnly | QFile::Truncate)) {
        errDevice << "Failed to open gains file with error:" << f.errorString();
        return false;
    }

    QTextStream s(&f);
    s << gains.size() << '\n';
    for (int i = 0; i < gains.size(); ++i) {
        s << gains.at(i) << '\n';
    }

    infoDevice << "Gains successfully saved";
    return true;
}

bool FlatFieldCorrection::calibrate(const QVector<float> &img,
                                    const QVector<float> &darkFrame,
                                    int width)
{
    infoDevice << "Starting calibration";

    if (width < 1) {
        errDevice << "Bad width param";
        return false;
    }

    if (img.isEmpty() || img.size() % width ||
        darkFrame.isEmpty() || darkFrame.size() % width) {
        errDevice << "Bad images sizes";
        return false;
    }

    QVector<float> darkFrameRates = ratesFromDarkFrame(darkFrame, width);
    const int height = img.size() / width;

    double avgN = 0;
    {
        double sum = 0;
        quint64 count = 0;
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < height; ++j) {
                if (img.at(width * j + i) >= darkFrameRates.at(i)) {
                    sum += static_cast<double>(img.at(width * j + i) - darkFrameRates.at(i));
                    ++count;
                }
            }
        }

        if (!count) {
            errDevice << "Bad image";
            return false;
        }

        avgN = sum / count;
    }

    if (qFuzzyIsNull(avgN)) {
        errDevice << "AvgN is equal to zero";
        return false;
    }

    QVector<float> gains;
    gains.reserve(width);

    for (int i = 0; i < width; ++i) {
        double sum = 0;
        quint64 count = 0;
        for (int j = 0; j < height; ++j) {
            if (img.at(width * j + i) >= darkFrameRates.at(i)) {
                sum += static_cast<double>(img.at(width * j + i) - darkFrameRates.at(i));
                ++count;
            }
        }

        if (!count) {
            errDevice << "Bad image";
            return false;
        }

        gains.append(static_cast<float>(sum / count / avgN));
    }

    if (!saveGains(gains)) {
        warnDevice << "Failed to save gains into file";
    }

    m_gains = gains;
    setLastCalibrationDateTime(QDateTime::currentDateTime());
    return true;
}

bool FlatFieldCorrection::correct(QVector<float> &img,
                                  const QVector<float> &darkFrame,
                                  int width)
{
    infoDevice << "Starting Flat Field Correction";

    if (width < 1 || m_gains.size() != width) {
        errDevice << "Bad gains or width param";
        return false;
    }

    if (img.isEmpty() || darkFrame.isEmpty() || img.size() % width || darkFrame.size() % width) {
        errDevice << "Bad images sizes";
        return false;
    }

    const QVector<float> darkFrameRates = ratesFromDarkFrame(darkFrame, width);
    const int height = img.size() / width;

    int minValue = 0;
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            const int index = j * width + i;
            int value = qCeil(static_cast<double>((img.at(index) - darkFrameRates.at(i)) / qAbs(m_gains.at(i))));
            if (value < minValue) {
                minValue = value;
            }

            img[index] = value;
        }
    }

    if (minValue) {
        for (int i = 0; i < img.size(); ++i) {
            img[i] -= minValue;
        }
    }

    return true;
}

void FlatFieldCorrection::setLastCalibrationDateTime(const QDateTime &lastCalibration)
{
    QMutexLocker locker(&m_lastCalibrationMutex);
    m_lastCalibration = lastCalibration;
}
