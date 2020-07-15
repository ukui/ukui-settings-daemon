#ifndef __USD_COMMON_KEYGRAB_H
#define __USD_COMMON_KEYGRAB_H
#include <QList>

#ifdef signals
#undef signals
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <gdk/gdk.h>
#include <glib.h>
#include <X11/keysym.h>
#include <X11/Xlib.h>

typedef struct {
        guint keysym;
        guint state;
        guint *keycodes;
} Key;


void	        grab_key_unsafe	(Key     *key,
                     bool grab,
                     QList<GdkScreen*>  *screens);

gboolean        match_key       (Key     *key,
                                 XEvent  *event);

gboolean        key_uses_keycode (const Key *key,
                                  guint keycode);

#ifdef __cplusplus
}
#endif

#endif /* __USD_COMMON_KEYGRAB_H */
