#ifndef KEYBOARD_GLOBAL_H
#define KEYBOARD_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(KEYBOARD_LIBRARY)
#  define KEYBOARD_EXPORT Q_DECL_EXPORT
#else
#  define KEYBOARD_EXPORT Q_DECL_IMPORT
#endif

#endif // KEYBOARD_GLOBAL_H
