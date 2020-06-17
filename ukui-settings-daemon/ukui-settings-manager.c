/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <gio/gio.h>

#include "ukui-settings-plugin-info.h"
#include "ukui-settings-manager.h"
#include "ukui-settings-manager-glue.h"
#include "ukui-settings-profile.h"

#define USD_MANAGER_DBUS_PATH "/org/ukui/SettingsDaemon"

#define DEFAULT_SETTINGS_PREFIX "org.ukui.SettingsDaemon"

#define PLUGIN_EXT ".ukui-settings-plugin"

#define UKUI_SETTINGS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UKUI_TYPE_SETTINGS_MANAGER, UkuiSettingsManagerPrivate))

struct UkuiSettingsManagerPrivate
{
        DBusGConnection            *connection;
        GSList                     *plugins;
};

enum {
        PLUGIN_ACTIVATED,
        PLUGIN_DEACTIVATED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     ukui_settings_manager_class_init  (UkuiSettingsManagerClass *klass);
static void     ukui_settings_manager_init        (UkuiSettingsManager      *settings_manager);
static void     ukui_settings_manager_finalize    (GObject                   *object);

G_DEFINE_TYPE (UkuiSettingsManager, ukui_settings_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

GQuark
ukui_settings_manager_error_quark (void)
{
        static GQuark ret = 0;
        if (ret == 0) {
                ret = g_quark_from_static_string ("ukui_settings_manager_error");
        }

        return ret;
}

static void
maybe_activate_plugin (UkuiSettingsPluginInfo *info, gpointer user_data)
{
        if (ukui_settings_plugin_info_get_enabled (info)) {
                gboolean res;
                res = ukui_settings_plugin_info_activate (info);
                if (res) {
                        g_debug ("Plugin %s: active", ukui_settings_plugin_info_get_location (info));
                } else {
                        g_debug ("Plugin %s: activation failed", ukui_settings_plugin_info_get_location (info));
                }
        } else {
                g_debug ("Plugin %s: inactive", ukui_settings_plugin_info_get_location (info));
        }
}

static gint
compare_location (UkuiSettingsPluginInfo *a,
                  UkuiSettingsPluginInfo *b)
{
        const char *loc_a;
        const char *loc_b;

        loc_a = ukui_settings_plugin_info_get_location (a);
        loc_b = ukui_settings_plugin_info_get_location (b);

        if (loc_a == NULL || loc_b == NULL) {
                return -1;
        }

        return strcmp (loc_a, loc_b);
}

static int
compare_priority (UkuiSettingsPluginInfo *a,
                  UkuiSettingsPluginInfo *b)
{
        int prio_a;
        int prio_b;

        prio_a = ukui_settings_plugin_info_get_priority (a);
        prio_b = ukui_settings_plugin_info_get_priority (b);

        return prio_a - prio_b;
}

static void
on_plugin_activated (UkuiSettingsPluginInfo *info,
                     UkuiSettingsManager    *manager)
{
        const char *name;
        name = ukui_settings_plugin_info_get_location (info);
        g_debug ("UkuiSettingsManager: emitting plugin-activated %s", name);
        g_signal_emit (manager, signals [PLUGIN_ACTIVATED], 0, name);
}

static void
on_plugin_deactivated (UkuiSettingsPluginInfo *info,
                       UkuiSettingsManager    *manager)
{
        const char *name;
        name = ukui_settings_plugin_info_get_location (info);
        g_debug ("UkuiSettingsManager: emitting plugin-deactivated %s", name);
        g_signal_emit (manager, signals [PLUGIN_DEACTIVATED], 0, name);
}

static gboolean
is_item_in_schema (const char * const *items,
                   const char         *item)
{
	while (*items) {
	       if (g_strcmp0 (*items++, item) == 0)
		       return TRUE;
	}
	return FALSE;
}

static gboolean
is_schema (const char *schema)
{
	return is_item_in_schema (g_settings_list_schemas (), schema);
}

static void
_load_file (UkuiSettingsManager *manager,
            const char           *filename)
{
        UkuiSettingsPluginInfo  *info;
        char                    *schema;
        GSList                  *l;

        g_debug ("Loading plugin: %s", filename);
        ukui_settings_profile_start ("%s", filename);

        info = ukui_settings_plugin_info_new_from_file (filename);
        if (info == NULL) {
                goto out;
        }

        l = g_slist_find_custom (manager->priv->plugins,
                                 info,
                                 (GCompareFunc) compare_location);
        if (l != NULL) {
                goto out;
        }

        schema = g_strdup_printf ("%s.plugins.%s",
                                  DEFAULT_SETTINGS_PREFIX,
                                  ukui_settings_plugin_info_get_location (info));

	/* Ignore unknown schemas or else we'll assert */
	if (is_schema (schema)) {
	       manager->priv->plugins = g_slist_prepend (manager->priv->plugins,
		                                         g_object_ref (info));

	       g_signal_connect (info, "activated",
		                 G_CALLBACK (on_plugin_activated), manager);
	       g_signal_connect (info, "deactivated",
		                 G_CALLBACK (on_plugin_deactivated), manager);

	       /* Also sets priority for plugins */
	       ukui_settings_plugin_info_set_schema (info, schema);
	} else {
	       g_warning ("Ignoring unknown module '%s'", schema);
	}

        g_free (schema);

 out:
        if (info != NULL) {
                g_object_unref (info);
        }

        ukui_settings_profile_end ("%s", filename);
}

static void
_load_dir (UkuiSettingsManager *manager,
           const char           *path)
{
        GError     *error;
        GDir       *d;
        const char *name;

        g_debug ("Loading settings plugins from dir: %s", path);
        ukui_settings_profile_start (NULL);

        error = NULL;
        d = g_dir_open (path, 0, &error);
        if (d == NULL) {
                g_warning ("%s", error->message);
                g_error_free (error);
                return;
        }

        while ((name = g_dir_read_name (d))) {
                char *filename;

                if (!g_str_has_suffix (name, PLUGIN_EXT)) {
                        continue;
                }

                filename = g_build_filename (path, name, NULL);
                if (g_file_test (filename, G_FILE_TEST_IS_REGULAR)) {
                        _load_file (manager, filename);
                }
                g_free (filename);
        }

        g_dir_close (d);

        ukui_settings_profile_end (NULL);
}

static void
_load_all (UkuiSettingsManager *manager)
{
        ukui_settings_profile_start (NULL);

        /* load system plugins */
        _load_dir (manager, UKUI_SETTINGS_PLUGINDIR G_DIR_SEPARATOR_S);

        manager->priv->plugins = g_slist_sort (manager->priv->plugins, (GCompareFunc) compare_priority);
        g_slist_foreach (manager->priv->plugins, (GFunc) maybe_activate_plugin, NULL);
        ukui_settings_profile_end (NULL);
}

static void
_unload_plugin (UkuiSettingsPluginInfo *info, gpointer user_data)
{
        if (ukui_settings_plugin_info_get_enabled (info)) {
                ukui_settings_plugin_info_deactivate (info);
        }
        g_object_unref (info);
}

static void
_unload_all (UkuiSettingsManager *manager)
{
         g_slist_foreach (manager->priv->plugins, (GFunc) _unload_plugin, NULL);
         g_slist_free (manager->priv->plugins);
         manager->priv->plugins = NULL;
}

/*
  Example:
  dbus-send --session --dest=org.ukui.SettingsDaemon \
  --type=method_call --print-reply --reply-timeout=2000 \
  /org/ukui/SettingsDaemon \
  org.ukui.SettingsDaemon.Awake
*/
gboolean
ukui_settings_manager_awake (UkuiSettingsManager *manager,
                              GError              **error)
{
        g_debug ("Awake called");
        return ukui_settings_manager_start (manager, error);
}

static gboolean
register_manager (UkuiSettingsManager *manager)
{
        GError *error = NULL;

        manager->priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (manager->priv->connection == NULL) {
                if (error != NULL) {
                        g_critical ("error getting system bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        dbus_g_connection_register_g_object (manager->priv->connection, USD_MANAGER_DBUS_PATH, G_OBJECT (manager));

        return TRUE;
}

gboolean
ukui_settings_manager_start (UkuiSettingsManager *manager,
                              GError              **error)
{
        gboolean ret;

        g_debug ("Starting settings manager");

        ret = FALSE;

        ukui_settings_profile_start (NULL);

        if (!g_module_supported ()) {
                g_warning ("ukui-settings-daemon is not able to initialize the plugins.");
                g_set_error (error,
                             UKUI_SETTINGS_MANAGER_ERROR,
                             UKUI_SETTINGS_MANAGER_ERROR_GENERAL,
                             "Plugins not supported");

                goto out;
        }

        _load_all (manager);

        ret = TRUE;
 out:
        ukui_settings_profile_end (NULL);

        return ret;
}

void
ukui_settings_manager_stop (UkuiSettingsManager *manager)
{
        g_debug ("Stopping settings manager");

        _unload_all (manager);
}

static void
ukui_settings_manager_dispose (GObject *object)
{
        UkuiSettingsManager *manager;

        manager = UKUI_SETTINGS_MANAGER (object);

        ukui_settings_manager_stop (manager);

        G_OBJECT_CLASS (ukui_settings_manager_parent_class)->dispose (object);
}

static void
ukui_settings_manager_class_init (UkuiSettingsManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->dispose = ukui_settings_manager_dispose;
        object_class->finalize = ukui_settings_manager_finalize;

        signals [PLUGIN_ACTIVATED] =
                g_signal_new ("plugin-activated",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (UkuiSettingsManagerClass, plugin_activated),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);
        signals [PLUGIN_DEACTIVATED] =
                g_signal_new ("plugin-deactivated",
                              G_TYPE_FROM_CLASS (object_class),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (UkuiSettingsManagerClass, plugin_deactivated),
                              NULL,
                              NULL,
                              g_cclosure_marshal_VOID__STRING,
                              G_TYPE_NONE,
                              1, G_TYPE_STRING);

        g_type_class_add_private (klass, sizeof (UkuiSettingsManagerPrivate));

        dbus_g_object_type_install_info (UKUI_TYPE_SETTINGS_MANAGER, &dbus_glib_ukui_settings_manager_object_info);
}

static void
ukui_settings_manager_init (UkuiSettingsManager *manager)
{

        manager->priv = UKUI_SETTINGS_MANAGER_GET_PRIVATE (manager);
}

static void
ukui_settings_manager_finalize (GObject *object)
{
        UkuiSettingsManager *manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (UKUI_IS_SETTINGS_MANAGER (object));

        manager = UKUI_SETTINGS_MANAGER (object);

        g_return_if_fail (manager->priv != NULL);

        G_OBJECT_CLASS (ukui_settings_manager_parent_class)->finalize (object);
}

UkuiSettingsManager *
ukui_settings_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                gboolean res;

                manager_object = g_object_new (UKUI_TYPE_SETTINGS_MANAGER,
                                               NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
                res = register_manager (manager_object);
                if (! res) {
                        g_object_unref (manager_object);
                        return NULL;
                }
        }

        return UKUI_SETTINGS_MANAGER (manager_object);
}
