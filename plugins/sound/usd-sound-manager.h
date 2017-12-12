/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Lennart Poettering <lennart@poettering.net>
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

#ifndef __USD_SOUND_MANAGER_H
#define __USD_SOUND_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_SOUND_MANAGER         (usd_sound_manager_get_type ())
#define USD_SOUND_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_SOUND_MANAGER, UsdSoundManager))
#define USD_SOUND_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), USD_TYPE_SOUND_MANAGER, UsdSoundManagerClass))
#define USD_IS_SOUND_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_SOUND_MANAGER))
#define USD_IS_SOUND_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_SOUND_MANAGER))
#define USD_SOUND_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_SOUND_MANAGER, UsdSoundManagerClass))

typedef struct UsdSoundManagerPrivate UsdSoundManagerPrivate;

typedef struct
{
        GObject parent;
        UsdSoundManagerPrivate *priv;
} UsdSoundManager;

typedef struct
{
        GObjectClass parent_class;
} UsdSoundManagerClass;

GType usd_sound_manager_get_type (void) G_GNUC_CONST;

UsdSoundManager *usd_sound_manager_new (void);
gboolean usd_sound_manager_start (UsdSoundManager *manager, GError **error);
void usd_sound_manager_stop (UsdSoundManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __USD_SOUND_MANAGER_H */
