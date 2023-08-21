#include "ScaleAcquisitionResultProcessor.h"

#include <opencv2/imgproc.hpp>

ScaleAcquisitionResultProcessor::ScaleAcquisitionResultProcessor() :
    m_width(0)
{

}

void ScaleAcquisitionResultProcessor::setWidth(int width)
{
    m_width = width;
}

void ScaleAcquisitionResultProcessor::process(Scanner::AcquisitionResult &result) const
{
    if (m_width < 1 || result.width < 1) {
        return;
    }

    QVector<float> &srcImg = result.image;
    int width = result.width;
    int height = srcImg.size() / width;

    QVector<float> dstImg(height * m_width);

    cv::Mat src(height, width, CV_32FC1, srcImg.data());
    cv::Mat dst(height, m_width, CV_32FC1, dstImg.data());
    cv::resize(src, dst, dst.size(), 0, 0, cv::INTER_CUBIC);

    result.width = m_width;
    result.image = dstImg;
}
