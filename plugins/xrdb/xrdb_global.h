#ifndef XRDB_GLOBAL_H
#define XRDB_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(XRDB_LIBRARY)
#  define XRDBSHARED_EXPORT Q_DECL_EXPORT
#else
#  define XRDBSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // XRDB_GLOBAL_H
