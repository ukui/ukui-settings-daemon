/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Lennart Poettering <lennart@poettering.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
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

#ifndef __USD_SOUND_PLUGIN_H__
#define __USD_SOUND_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include "ukui-settings-plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_SOUND_PLUGIN                (usd_sound_plugin_get_type ())
#define USD_SOUND_PLUGIN(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_SOUND_PLUGIN, UsdSoundPlugin))
#define USD_SOUND_PLUGIN_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST ((k), USD_TYPE_SOUND_PLUGIN, UsdSoundPluginClass))
#define USD_IS_SOUND_PLUGIN(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_SOUND_PLUGIN))
#define USD_IS_SOUND_PLUGIN_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_SOUND_PLUGIN))
#define USD_SOUND_PLUGIN_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_SOUND_PLUGIN, UsdSoundPluginClass))

typedef struct UsdSoundPluginPrivate UsdSoundPluginPrivate;

typedef struct
{
        UkuiSettingsPlugin parent;
        UsdSoundPluginPrivate *priv;
} UsdSoundPlugin;

typedef struct
{
        UkuiSettingsPluginClass parent_class;
} UsdSoundPluginClass;

GType usd_sound_plugin_get_type (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_ukui_settings_plugin (GTypeModule *module);

#ifdef __cplusplus
}
#endif

#endif /* __USD_SOUND_PLUGIN_H__ */
