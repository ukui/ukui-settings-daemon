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

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include "ukui-settings-plugin.h"
#include "usd-sound-plugin.h"
#include "usd-sound-manager.h"

struct UsdSoundPluginPrivate {
        UsdSoundManager *manager;
};

#define USD_SOUND_PLUGIN_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), USD_TYPE_SOUND_PLUGIN, UsdSoundPluginPrivate))

UKUI_SETTINGS_PLUGIN_REGISTER (UsdSoundPlugin, usd_sound_plugin)

static void
usd_sound_plugin_init (UsdSoundPlugin *plugin)
{
        plugin->priv = USD_SOUND_PLUGIN_GET_PRIVATE (plugin);

        g_debug ("UsdSoundPlugin initializing");

        plugin->priv->manager = usd_sound_manager_new ();
}

static void
usd_sound_plugin_finalize (GObject *object)
{
        UsdSoundPlugin *plugin;

        g_return_if_fail (object != NULL);
        g_return_if_fail (USD_IS_SOUND_PLUGIN (object));

        g_debug ("UsdSoundPlugin finalizing");

        plugin = USD_SOUND_PLUGIN (object);

        g_return_if_fail (plugin->priv != NULL);

        if (plugin->priv->manager != NULL)
                g_object_unref (plugin->priv->manager);

        G_OBJECT_CLASS (usd_sound_plugin_parent_class)->finalize (object);
}

static void
impl_activate (UkuiSettingsPlugin *plugin)
{
        GError *error = NULL;

        g_debug ("Activating sound plugin");

        if (!usd_sound_manager_start (USD_SOUND_PLUGIN (plugin)->priv->manager, &error)) {
                g_warning ("Unable to start sound manager: %s", error->message);
                g_error_free (error);
        }
}

static void
impl_deactivate (UkuiSettingsPlugin *plugin)
{
        g_debug ("Deactivating sound plugin");
        usd_sound_manager_stop (USD_SOUND_PLUGIN (plugin)->priv->manager);
}

static void
usd_sound_plugin_class_init (UsdSoundPluginClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        UkuiSettingsPluginClass *plugin_class = UKUI_SETTINGS_PLUGIN_CLASS (klass);

        object_class->finalize = usd_sound_plugin_finalize;

        plugin_class->activate = impl_activate;
        plugin_class->deactivate = impl_deactivate;

        g_type_class_add_private (klass, sizeof (UsdSoundPluginPrivate));
}

static void
usd_sound_plugin_class_finalize (UsdSoundPluginClass *klass)
{
}

