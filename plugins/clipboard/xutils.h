#ifndef X_UTILS_H
#define X_UTILS_H

#include <X11/Xlib.h>

#ifdef __cplusplus
extern "C" {
#endif

extern Atom XA_ATOM_PAIR;
extern Atom XA_CLIPBOARD_MANAGER;
extern Atom XA_CLIPBOARD;
extern Atom XA_DELETE;
extern Atom XA_INCR;
extern Atom XA_INSERT_PROPERTY;
extern Atom XA_INSERT_SELECTION;
extern Atom XA_MANAGER;
extern Atom XA_MULTIPLE;
extern Atom XA_NULL;
extern Atom XA_SAVE_TARGETS;
extern Atom XA_TARGETS;
extern Atom XA_TIMESTAMP;

extern unsigned long SELECTION_MAX_SIZE;

void init_atoms      (Display *display);

Time get_server_time (Display *display,
		      Window   window);

#ifdef __cplusplus
}
#endif
#endif /* X_UTILS_H */
