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
