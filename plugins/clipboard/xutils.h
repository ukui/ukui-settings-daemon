/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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
