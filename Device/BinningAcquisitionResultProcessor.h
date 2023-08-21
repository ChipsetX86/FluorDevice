#ifndef DEVICE_BINNINGACQUISITIONRESULTPROCESSOR_H
#define DEVICE_BINNINGACQUISITIONRESULTPROCESSOR_H

#include "ScannerAcquisitionResultProcessor.h"

class DEVICELIB_EXPORT BinningAcquisitionResultProcessor : public ScannerAcquisitionResultProcessor
{
public:
    BinningAcquisitionResultProcessor();

    void setHorizontal(int x);
    void setVertical(int y);
    void setSum(bool sum);
    
    void process(Scanner::AcquisitionResult &result) const override;
private:
    int m_x;
    int m_y;
    bool m_sum;
};

#endif // DEVICE_BINNINGACQUISITIONRESULTPROCESSOR_H
