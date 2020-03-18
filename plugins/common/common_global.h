#ifndef COMMON_GLOBAL_H
#define COMMON_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(COMMON_LIBRARY)
#  define COMMONSHARED_EXPORT Q_DECL_EXPORT
#else
#  define COMMONSHARED_EXPORT Q_DECL_IMPORT
#endif


#define DIALOG_TIMEOUT 2000     /* dialog timeout in ms */
#define DIALOG_FADE_TIMEOUT 1500 /* timeout before fade starts */
#define FADE_TIMEOUT 10        /* timeout in ms between each frame of the fade */

#define BG_ALPHA 0.75

#endif // COMMON_GLOBAL_H
