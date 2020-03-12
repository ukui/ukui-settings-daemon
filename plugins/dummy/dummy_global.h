#ifndef DUMMY_GLOBAL_H
#define DUMMY_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(DUMMY_LIBRARY)
#  define DUMMY_EXPORT Q_DECL_EXPORT
#else
#  define DUMMY_EXPORT Q_DECL_IMPORT
#endif

#endif // DUMMY_GLOBAL_H
