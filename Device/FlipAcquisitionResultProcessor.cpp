#include "FlipAcquisitionResultProcessor.h"

FlipAcquisitionResultProcessor::FlipAcquisitionResultProcessor() :
    m_flipHorizontal(false)
{

}

void FlipAcquisitionResultProcessor::setFlipHorizontal(bool flip)
{
    m_flipHorizontal = flip;
}

void FlipAcquisitionResultProcessor::process(Scanner::AcquisitionResult &result) const
{
    if (!m_flipHorizontal || result.width < 1) {
        return;
    }

    QVector<float> &img = result.image;
    int width = result.width;
    int height = img.size() / width;

    for (int i = 0; i < height; ++i) {
        for (int j = 0; j < width / 2; ++j) {
            std::swap(img[i * width + j], img[i * width + width - j - 1]);
        }
    }
}
