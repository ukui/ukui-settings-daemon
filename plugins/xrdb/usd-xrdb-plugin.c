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

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include "ukui-settings-plugin.h"
#include "usd-xrdb-plugin.h"
#include "usd-xrdb-manager.h"

struct UsdXrdbPluginPrivate {
        UsdXrdbManager *manager;
};

#define USD_XRDB_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), USD_TYPE_XRDB_PLUGIN, UsdXrdbPluginPrivate))

UKUI_SETTINGS_PLUGIN_REGISTER (UsdXrdbPlugin, usd_xrdb_plugin)

static void
usd_xrdb_plugin_init (UsdXrdbPlugin *plugin)
{
        plugin->priv = USD_XRDB_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("UsdXrdbPlugin initializing");

        plugin->priv->manager = usd_xrdb_manager_new ();
}

static void
usd_xrdb_plugin_finalize (GObject *object)
{
        UsdXrdbPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (USD_IS_XRDB_PLUGIN (object));

        g_debug ("UsdXrdbPlugin finalizing");

        plugin = USD_XRDB_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL) {
                g_object_unref (plugin->priv->manager);
        }

        G_OBJECT_CLASS (usd_xrdb_plugin_parent_class)->finalize (object);
}

static void
impl_activate (UkuiSettingsPlugin *plugin)
{
        gboolean res;
        GError  *error;

        g_debug ("Activating xrdb plugin");

        error = NULL;
        res = usd_xrdb_manager_start (USD_XRDB_PLUGIN (plugin)->priv->manager, &error);
        if (! res) {
                g_warning ("Unable to start xrdb manager: %s", error->message);
                g_error_free (error);
        }
}

static void
impl_deactivate (UkuiSettingsPlugin *plugin)
{
        g_debug ("Deactivating xrdb plugin");
        usd_xrdb_manager_stop (USD_XRDB_PLUGIN (plugin)->priv->manager);
}

static void
usd_xrdb_plugin_class_init (UsdXrdbPluginClass *klass)
{
        GObjectClass             *object_class = G_OBJECT_CLASS (klass);
        UkuiSettingsPluginClass *plugin_class = UKUI_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = usd_xrdb_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (UsdXrdbPluginPrivate));
}

static void
usd_xrdb_plugin_class_finalize (UsdXrdbPluginClass *klass)
{
}

