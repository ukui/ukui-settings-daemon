/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#ifndef __USD_BACKGROUND_PLUGIN_H__
#define __USD_BACKGROUND_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

#include "ukui-settings-plugin.h"

#ifdef __cplusplus
extern "C" {
#endif

//class UsdBackgroundPlugin
//{
	#define USD_TYPE_BACKGROUND_PLUGIN                (usd_background_plugin_get_type())
	#define USD_BACKGROUND_PLUGIN(o)                  (G_TYPE_CHECK_INSTANCE_CAST((o), USD_TYPE_BACKGROUND_PLUGIN, UsdBackgroundPlugin))
	#define USD_BACKGROUND_PLUGIN_CLASS(k)            (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_BACKGROUND_PLUGIN, UsdBackgroundPluginClass))
	#define USD_IS_BACKGROUND_PLUGIN(o)               (G_TYPE_CHECK_INSTANCE_TYPE((o), USD_TYPE_BACKGROUND_PLUGIN))
	#define USD_IS_BACKGROUND_PLUGIN_CLASS(k)         (G_TYPE_CHECK_CLASS_TYPE((k), USD_TYPE_BACKGROUND_PLUGIN))
	#define USD_BACKGROUND_PLUGIN_GET_CLASS(o)        (G_TYPE_INSTANCE_GET_CLASS((o), USD_TYPE_BACKGROUND_PLUGIN, UsdBackgroundPluginClass))

	typedef struct UsdBackgroundPluginPrivate UsdBackgroundPluginPrivate;

	typedef struct {
		UkuiSettingsPlugin          parent;
		UsdBackgroundPluginPrivate* priv;
	} UsdBackgroundPlugin;

	typedef struct {
		UkuiSettingsPluginClass parent_class;
	} UsdBackgroundPluginClass;

	GType
	usd_background_plugin_get_type (void) G_GNUC_CONST;

	/* All the plugins must implement this function */
	G_MODULE_EXPORT GType
	register_ukui_settings_plugin (GTypeModule* module);
//}

#ifdef __cplusplus
}
#endif

#endif /* __USD_BACKGROUND_PLUGIN_H__ */
