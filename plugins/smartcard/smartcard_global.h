#ifndef SMARTCARD_GLOBAL_H
#define SMARTCARD_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(SMARTCARD_LIBRARY)
#  define SMARTCARD_EXPORT Q_DECL_EXPORT
#else
#  define SMARTCARD_EXPORT Q_DECL_IMPORT
#endif

#endif // SMARTCARD_GLOBAL_H
