#include "ukuisettingsmanager.h"
#include "ukuisettingsplugininfo.h"
#include "ukuisettingsprofile.h"
#include <gio/gio.h>
#include <dbus/dbus-glib.h>
#include <glib.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib/gstdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define USD_MANAGER_DBUS_PATH "/org/ukui/SettingsDaemon"
#define DEFAULT_SETTINGS_PREFIX "org.ukui.SettingsDaemon"
#define PLUGIN_EXT ".ukui-settings-plugin"

UkuiSettingsManager* UkuiSettingsManager::mUkuiSettingsManager = NULL;
DBusGConnection* UkuiSettingsManager::connection = NULL;

UkuiSettingsManager::UkuiSettingsManager()
{

}

UkuiSettingsManager::~UkuiSettingsManager()
{

}

UkuiSettingsManager* UkuiSettingsManager::ukuiSettingsManagerNew()
{
    if (nullptr == mUkuiSettingsManager) {
        mUkuiSettingsManager = new UkuiSettingsManager;
    }

    // register
    registerManager();

    return mUkuiSettingsManager;
}

gboolean UkuiSettingsManager::ukuiSettingsManagerStart(GError **error)
{
    gboolean ret;

    g_debug ("Starting settings manager");

    loadAll();

    return TRUE;
}

void UkuiSettingsManager::ukuiSettingsManagerStop()
{
    g_debug ("Stopping settings manager");
    // unloadAll()
}

gboolean UkuiSettingsManager::ukuiSettingsManagerAwake()
{
    return TRUE;
}

void UkuiSettingsManager::pluginActivated(QString name)
{

}

void UkuiSettingsManager::pluginDeactivated(QString name)
{

}

gboolean UkuiSettingsManager::registerManager()
{
    GError* error = NULL;
    connection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (NULL == connection) {
        if (NULL == error) {
            g_critical("error getting system bus: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }
    dbus_g_connection_register_g_object(connection, USD_MANAGER_DBUS_PATH, NULL);

    return TRUE;
}

// FIXME://
void UkuiSettingsManager::loadAll()
{
    // ukui_settings_profile_start (NULL);
    QString p("/data/p");
    loadDir (p);

    // ukui_settings_profile_end (NULL);
}

void UkuiSettingsManager::loadDir(QString &path)
{
    GError*     error = NULL;
    GDir*       dir = NULL;
    const char* name = NULL;

    g_debug ("Loading settings plugins from dir: %s", (char*)path.data());

    // ukui_settings_profile_start (NULL);
    dir = g_dir_open ((char*)path.data(), 0, &error);
    if (NULL == dir) {
        g_warning("%s", error->message);
        g_error_free(error);
        return;
    }

    while ((name = g_dir_read_name(dir))) {
        char* filename = NULL;
        if (!g_str_has_suffix(name, PLUGIN_EXT)) {
            continue;
        }
        filename = g_build_filename((char*)path.data(), name, NULL);
        if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
            QString ftmp(filename);
            loadFile(ftmp);
        }
        g_free(filename);
    }
    g_dir_close(dir);
    // ukui_settings_profile_end (NULL);
}

void UkuiSettingsManager::loadFile(QString &fileName)
{
    UkuiSettingsPluginInfo* info = NULL;
    char*                   schema = NULL;
    GSList*                 l = NULL;

    g_debug("Loading plugin: %s", fileName.data());

    // ukui_settings_profile_start ("%s", filename);

    /*
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
    if (is_schema (schema)) {
           manager->priv->plugins = g_slist_prepend (manager->priv->plugins,
                                                 g_object_ref (info));

           g_signal_connect (info, "activated",
                         G_CALLBACK (on_plugin_activated), manager);
           g_signal_connect (info, "deactivated",
                         G_CALLBACK (on_plugin_deactivated), manager);

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
     */
}
