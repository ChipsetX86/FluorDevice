#ifndef DEVICE_SCANNERACQUISITIONRESULTPROCESSOR_H
#define DEVICE_SCANNERACQUISITIONRESULTPROCESSOR_H

#include "DeviceGlobal.h"
#include "Scanner.h"

class DEVICELIB_EXPORT ScannerAcquisitionResultProcessor
{
public:
    virtual ~ScannerAcquisitionResultProcessor();
    
    virtual void process(Scanner::AcquisitionResult &result) const = 0;
};

#endif // DEVICE_SCANNERACQUISITIONRESULTPROCESSOR_H
