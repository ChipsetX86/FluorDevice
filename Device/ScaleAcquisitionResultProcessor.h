#ifndef DEVICE_SCALEACQUISITIONRESULTPROCESSOR_H
#define DEVICE_SCALEACQUISITIONRESULTPROCESSOR_H

#include "ScannerAcquisitionResultProcessor.h"

class DEVICELIB_EXPORT ScaleAcquisitionResultProcessor : public ScannerAcquisitionResultProcessor
{
public:
    ScaleAcquisitionResultProcessor();

    void setWidth(int width);
    
    void process(Scanner::AcquisitionResult &result) const override;
private:
    int m_width;
};

#endif // DEVICE_SCALEACQUISITIONRESULTPROCESSOR_H
