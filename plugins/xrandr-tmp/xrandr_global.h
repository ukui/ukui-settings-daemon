#ifndef XRANDR_GLOBAL_H
#define XRANDR_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(XRANDR_LIBRARY)
#  define XRANDR_EXPORT Q_DECL_EXPORT
#else
#  define XRANDR_EXPORT Q_DECL_IMPORT
#endif

#endif // XRANDR_GLOBAL_H
