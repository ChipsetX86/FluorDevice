#ifndef DEVICE_FLIPACQUISITIONRESULTPROCESSOR_H
#define DEVICE_FLIPACQUISITIONRESULTPROCESSOR_H

#include "ScannerAcquisitionResultProcessor.h"

class DEVICELIB_EXPORT FlipAcquisitionResultProcessor : public ScannerAcquisitionResultProcessor
{
public:
    FlipAcquisitionResultProcessor();

    void setFlipHorizontal(bool flip);

    void process(Scanner::AcquisitionResult &result) const override;
private:
    bool m_flipHorizontal;
};

#endif // DEVICE_FLIPACQUISITIONRESULTPROCESSOR_H
