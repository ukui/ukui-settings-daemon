/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 8; tab-width: 8 -*-
 *
 * On-screen-display (OSD) window for ukui-settings-daemon's plugins
 *
 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2009 Novell, Inc
 *
 * Authors:
 *   William Jon McCann <mccann@jhu.edu>
 *   Federico Mena-Quintero <federico@novell.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this program; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

/* UsdOsdWindow is an "on-screen-display" window (OSD).  It is the cute,
 * semi-transparent, curved popup that appears when you press a hotkey global to
 * the desktop, such as to change the volume, switch your monitor's parameters,
 * etc.
 *
 * You can create a UsdOsdWindow and use it as a normal GtkWindow.  It will
 * automatically center itself, figure out if it needs to be composited, etc.
 * Just pack your widgets in it, sit back, and enjoy the ride.
 */

#ifndef USD_OSD_WINDOW_H
#define USD_OSD_WINDOW_H

#include <glib-object.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Alpha value to be used for foreground objects drawn in an OSD window */
#define USD_OSD_WINDOW_FG_ALPHA 1.0

#define USD_TYPE_OSD_WINDOW            (usd_osd_window_get_type ())
#define USD_OSD_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  USD_TYPE_OSD_WINDOW, UsdOsdWindow))
#define USD_OSD_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),   USD_TYPE_OSD_WINDOW, UsdOsdWindowClass))
#define USD_IS_OSD_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  USD_TYPE_OSD_WINDOW))
#define USD_IS_OSD_WINDOW_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), USD_TYPE_OSD_WINDOW))
#define USD_OSD_WINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), USD_TYPE_OSD_WINDOW, UsdOsdWindowClass))

typedef struct UsdOsdWindow                   UsdOsdWindow;
typedef struct UsdOsdWindowClass              UsdOsdWindowClass;
typedef struct UsdOsdWindowPrivate            UsdOsdWindowPrivate;

struct UsdOsdWindow {
        GtkWindow                   parent;

        UsdOsdWindowPrivate  *priv;
};

struct UsdOsdWindowClass {
        GtkWindowClass parent_class;

        void (* draw_when_composited) (UsdOsdWindow *window, cairo_t *cr);
};

GType                 usd_osd_window_get_type          (void);

GtkWidget *           usd_osd_window_new               (void);
gboolean              usd_osd_window_is_composited     (UsdOsdWindow      *window);
gboolean              usd_osd_window_is_valid          (UsdOsdWindow      *window);
void                  usd_osd_window_update_and_hide   (UsdOsdWindow      *window);

#ifdef __cplusplus
}
#endif

#endif
