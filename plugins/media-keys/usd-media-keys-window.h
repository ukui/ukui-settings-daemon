/* -*- Mode: C; indent-tabs-mode: nil; c-basic-offset: 8; tab-width: 8 -*-
 *
 * Copyright (C) 2006 William Jon McCann <mccann@jhu.edu>
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

#ifndef USD_MEDIA_KEYS_WINDOW_H
#define USD_MEDIA_KEYS_WINDOW_H

#include <glib-object.h>
#include <gtk/gtk.h>

#include "usd-osd-window.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_MEDIA_KEYS_WINDOW            (usd_media_keys_window_get_type ())
#define USD_MEDIA_KEYS_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),  USD_TYPE_MEDIA_KEYS_WINDOW, UsdMediaKeysWindow))
#define USD_MEDIA_KEYS_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),   USD_TYPE_MEDIA_KEYS_WINDOW, UsdMediaKeysWindowClass))
#define USD_IS_MEDIA_KEYS_WINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),  USD_TYPE_MEDIA_KEYS_WINDOW))
#define USD_IS_MEDIA_KEYS_WINDOW_CLASS(klass) (G_TYPE_INSTANCE_GET_CLASS ((klass), USD_TYPE_MEDIA_KEYS_WINDOW))

typedef struct UsdMediaKeysWindow                   UsdMediaKeysWindow;
typedef struct UsdMediaKeysWindowClass              UsdMediaKeysWindowClass;
typedef struct UsdMediaKeysWindowPrivate            UsdMediaKeysWindowPrivate;

struct UsdMediaKeysWindow {
        UsdOsdWindow parent;

        UsdMediaKeysWindowPrivate  *priv;
};

struct UsdMediaKeysWindowClass {
        UsdOsdWindowClass parent_class;
};

typedef enum {
        USD_MEDIA_KEYS_WINDOW_ACTION_VOLUME,
        USD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM
} UsdMediaKeysWindowAction;

GType                 usd_media_keys_window_get_type          (void);

GtkWidget *           usd_media_keys_window_new               (void);
void                  usd_media_keys_window_set_action        (UsdMediaKeysWindow      *window,
                                                               UsdMediaKeysWindowAction action);
void                  usd_media_keys_window_set_action_custom (UsdMediaKeysWindow      *window,
                                                               const char              *icon_name,
                                                               gboolean                 show_level);
void                  usd_media_keys_window_set_volume_muted  (UsdMediaKeysWindow      *window,
                                                               gboolean                 muted);
void                  usd_media_keys_window_set_volume_level  (UsdMediaKeysWindow      *window,
                                                               int                      level);
gboolean              usd_media_keys_window_is_valid          (UsdMediaKeysWindow      *window);

#ifdef __cplusplus
}
#endif

#endif
