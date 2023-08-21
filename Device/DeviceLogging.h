#ifndef DEVICELOGGING_H
#define DEVICELOGGING_H

#include "DeviceGlobal.h"

#include <QLoggingCategory>

DEVICELIB_EXPORT Q_DECLARE_LOGGING_CATEGORY(DeviceLogging)

#define dbgDevice qCDebug(DeviceLogging)
#define warnDevice qCWarning(DeviceLogging)
#define errDevice qCCritical(DeviceLogging)
#define infoDevice qCInfo(DeviceLogging)

#endif // DEVICELOGGING_H
