#ifndef DEVICEGLOBAL_H
#define DEVICEGLOBAL_H

#include <QtGlobal>

#if defined(DEVICE_LIBRARY)
#  define DEVICELIB_EXPORT Q_DECL_EXPORT
#else
#  define DEVICELIB_EXPORT Q_DECL_IMPORT
#endif

#endif // DEVICEGLOBAL_H
