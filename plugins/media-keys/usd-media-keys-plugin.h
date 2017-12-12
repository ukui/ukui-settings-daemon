/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2014 Michal Ratajsky <michal.ratajsky@gmail.com>
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

#ifndef __USD_MEDIA_KEYS_PLUGIN_H__
#define __USD_MEDIA_KEYS_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include "ukui-settings-plugin.h"

G_BEGIN_DECLS

#define USD_TYPE_MEDIA_KEYS_PLUGIN                (usd_media_keys_plugin_get_type ())
#define USD_MEDIA_KEYS_PLUGIN(o)                  (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_MEDIA_KEYS_PLUGIN, UsdMediaKeysPlugin))
#define USD_MEDIA_KEYS_PLUGIN_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_MEDIA_KEYS_PLUGIN, UsdMediaKeysPluginClass))
#define USD_IS_MEDIA_KEYS_PLUGIN(o)               (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_MEDIA_KEYS_PLUGIN))
#define USD_IS_MEDIA_KEYS_PLUGIN_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_MEDIA_KEYS_PLUGIN))
#define USD_MEDIA_KEYS_PLUGIN_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_MEDIA_KEYS_PLUGIN, UsdMediaKeysPluginClass))

typedef struct _UsdMediaKeysPlugin         UsdMediaKeysPlugin;
typedef struct _UsdMediaKeysPluginClass    UsdMediaKeysPluginClass;
typedef struct _UsdMediaKeysPluginPrivate  UsdMediaKeysPluginPrivate;

struct _UsdMediaKeysPlugin
{
        UkuiSettingsPlugin          parent;
        UsdMediaKeysPluginPrivate  *priv;
};

struct _UsdMediaKeysPluginClass
{
        UkuiSettingsPluginClass     parent_class;
};

GType usd_media_keys_plugin_get_type (void) G_GNUC_CONST;

/* All the plugins must implement this function */
G_MODULE_EXPORT GType register_ukui_settings_plugin (GTypeModule *module);

G_END_DECLS

#endif /* __USD_MEDIA_KEYS_PLUGIN_H__ */
