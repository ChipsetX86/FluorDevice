#include "BinningAcquisitionResultProcessor.h"

BinningAcquisitionResultProcessor::BinningAcquisitionResultProcessor() :
    m_x(0),
    m_y(0),
    m_sum(false)
{

}

void BinningAcquisitionResultProcessor::setHorizontal(int x)
{
    m_x = x;
}

void BinningAcquisitionResultProcessor::setVertical(int y)
{
    m_y = y;
}

void BinningAcquisitionResultProcessor::setSum(bool sum)
{
    m_sum = sum;
}

void BinningAcquisitionResultProcessor::process(Scanner::AcquisitionResult &result) const
{
    if (result.width > 0 && m_x >= 0 && m_y >= 0 && (m_x >= 1 || m_y >= 1)) {
        QVector<float> binningResult;
        const QVector<float> &binningSource = result.image;
        const int width = result.width;
        const int height = binningSource.size() / width;
        int binningHeight = 0;

        for (int y = 0; y < height; y += (m_y + 1)) {
            for (int x = 0; x < width; x += (m_x + 1)) {
                float value = 0;
                const int remainsY = qMin(height - y - 1, m_y) + 1;
                const int remainsX = qMin(width - x - 1, m_x) + 1;
                const int remains  = remainsY * remainsX;
                for (int yy = 0; yy < remainsY; ++yy) {
                    for (int xx = 0; xx < remainsX; ++xx) {
                        value += binningSource.at((y + yy) * width + x + xx) / (m_sum ? 1 : remains);
                    }
                }

                binningResult.append(value);
            }

            binningHeight++;
        }

        QSizeF binningPixelSize = result.pixelSize;
        binningPixelSize.setWidth(binningPixelSize.width() * (m_x + 1));
        binningPixelSize.setHeight(binningPixelSize.height() * (m_y + 1));

        result.image = binningResult;
        result.width = binningResult.size() / binningHeight;
        result.pixelSize = binningPixelSize;
    }
}
