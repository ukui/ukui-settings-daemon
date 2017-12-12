/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2014 Michal Ratajsky <michal.ratajsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef __USD_MEDIA_KEYS_MANAGER_H
#define __USD_MEDIA_KEYS_MANAGER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define USD_TYPE_MEDIA_KEYS_MANAGER         (usd_media_keys_manager_get_type ())
#define USD_MEDIA_KEYS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_MEDIA_KEYS_MANAGER, UsdMediaKeysManager))
#define USD_MEDIA_KEYS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_MEDIA_KEYS_MANAGER, UsdMediaKeysManagerClass))
#define USD_IS_MEDIA_KEYS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_MEDIA_KEYS_MANAGER))
#define USD_IS_MEDIA_KEYS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_MEDIA_KEYS_MANAGER))
#define USD_MEDIA_KEYS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_MEDIA_KEYS_MANAGER, UsdMediaKeysManagerClass))

typedef struct _UsdMediaKeysManager         UsdMediaKeysManager;
typedef struct _UsdMediaKeysManagerClass    UsdMediaKeysManagerClass;
typedef struct _UsdMediaKeysManagerPrivate  UsdMediaKeysManagerPrivate;

struct _UsdMediaKeysManager
{
        GObject                     parent;
        UsdMediaKeysManagerPrivate *priv;
};

struct _UsdMediaKeysManagerClass
{
        GObjectClass   parent_class;
        void          (* media_player_key_pressed) (UsdMediaKeysManager *manager,
                                                    const char          *application,
                                                    const char          *key);
};

GType                 usd_media_keys_manager_get_type                  (void);

UsdMediaKeysManager * usd_media_keys_manager_new                       (void);
gboolean              usd_media_keys_manager_start                     (UsdMediaKeysManager *manager,
                                                                        GError             **error);
void                  usd_media_keys_manager_stop                      (UsdMediaKeysManager *manager);

gboolean              usd_media_keys_manager_grab_media_player_keys    (UsdMediaKeysManager *manager,
                                                                        const char          *application,
                                                                        guint32              time,
                                                                        GError             **error);
gboolean              usd_media_keys_manager_release_media_player_keys (UsdMediaKeysManager *manager,
                                                                        const char          *application,
                                                                        GError             **error);

G_END_DECLS

#endif /* __USD_MEDIA_KEYS_MANAGER_H */
