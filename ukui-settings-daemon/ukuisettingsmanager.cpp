#include "ukuisettingsmanager.h"
#include "ukuisettingsprofile.h"
#include "ukuisettingsplugininfo.h"

#include "global.h"
#include "clib_syslog.h"

#include <glib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib/gstdio.h>
#include <dbus/dbus-glib.h>

#include <QDebug>

DBusGConnection* UkuiSettingsManager::mConnection = NULL;
QList<UkuiSettingsPluginInfo*>* UkuiSettingsManager::mPlugin = NULL;
UkuiSettingsManager* UkuiSettingsManager::mUkuiSettingsManager = NULL;

bool is_schema (const char* schema);

UkuiSettingsManager::UkuiSettingsManager()
{

}

UkuiSettingsManager::~UkuiSettingsManager()
{

}

UkuiSettingsManager* UkuiSettingsManager::ukuiSettingsManagerNew()
{
    if (nullptr == mUkuiSettingsManager) {
        CT_SYSLOG(LOG_DEBUG, "ukui settings manager will be created!")
        mUkuiSettingsManager = new UkuiSettingsManager;
        mPlugin = new QList<UkuiSettingsPluginInfo*>();
    }

    registerManager();

    return mUkuiSettingsManager;
}

gboolean UkuiSettingsManager::ukuiSettingsManagerStart(GError **error)
{
    gboolean ret;

    CT_SYSLOG(LOG_DEBUG, "Starting settings manager");

    // FIXME:// maybe check system if support for plugin functionality

    loadAll();

    return TRUE;
}

void UkuiSettingsManager::ukuiSettingsManagerStop()
{
    CT_SYSLOG(LOG_DEBUG, "Stopping settings manager");
    // unloadAll()
}

gboolean UkuiSettingsManager::ukuiSettingsManagerAwake()
{
    return TRUE;
}

void UkuiSettingsManager::onPluginActivated(QString &name)
{
    CT_SYSLOG(LOG_DEBUG, "emitting plugin-activated '%s'", name.toLatin1().data());
    emit pluginActivated(name);
}

void UkuiSettingsManager::onPluginDeactivated(QString &name)
{
    CT_SYSLOG(LOG_DEBUG, "emitting plugin-deactivated '%s'", name.toLatin1().data());
    emit pluginDeactivated(name);
}

gboolean UkuiSettingsManager::registerManager()
{
    GError* error = NULL;
    mConnection = dbus_g_bus_get(DBUS_BUS_SESSION, &error);
    if (NULL == mConnection) {
        if (NULL == error) {
            CT_SYSLOG(LOG_ERR, "error getting system bus: %s", error->message);
            g_error_free(error);
        }
        return FALSE;
    }
    dbus_g_connection_register_g_object(mConnection, USD_MANAGER_DBUS_PATH, NULL);

    CT_SYSLOG(LOG_DEBUG, "regist settings manager successful!")
    return TRUE;
}

void UkuiSettingsManager::loadAll()
{
    QString p(UKUI_SETTINGS_PLUGINDIR);
    loadDir (p);
    //FIXME://
}

void UkuiSettingsManager::loadDir(QString &path)
{
    GError*     error = NULL;
    GDir*       dir = NULL;
    const char* name = NULL;

    CT_SYSLOG(LOG_DEBUG, "Loading settings plugins from dir: %s", (char*)path.toLatin1().data());

    dir = g_dir_open ((char*)path.toLatin1().data(), 0, &error);
    if (NULL == dir) {
        CT_SYSLOG(LOG_ERR, "%s", error->message);
        g_error_free(error);
        return;
    }

    while ((name = g_dir_read_name(dir))) {
        char* filename = NULL;
        if (!g_str_has_suffix(name, PLUGIN_EXT)) {
            continue;
        }
        filename = g_build_filename((char*)path.toLatin1().data(), name, NULL);
        if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
            QString ftmp(filename);
            loadFile(ftmp);
        }
        g_free(filename);
    }
    g_dir_close(dir);
}

void UkuiSettingsManager::loadFile(QString &fileName)
{
    UkuiSettingsPluginInfo* info = NULL;
    char*                   schema = NULL;
    GSList*                 l = NULL;

    CT_SYSLOG(LOG_DEBUG, "Loading plugin: %s", fileName.toLatin1().data());

    info = new UkuiSettingsPluginInfo(fileName);
    if (info == NULL) {
        goto out;
    }

    // can find ?
    // FIXME:// operator== in UkuiSettingsPluginInfo
    if (mPlugin->contains(info)) {
        CT_SYSLOG(LOG_DEBUG, "The list has contain this plugin, '%s'", fileName.toLatin1().data());
        goto out;
    }

    // check plugin's schema
    schema = g_strdup_printf ("%s.plugins.%s", DEFAULT_SETTINGS_PREFIX, info->ukuiSettingsPluginInfoGetLocation());
    if (is_schema (schema)) {
       mPlugin->insert(0, info);
       QObject::connect(info, SIGNAL(activated), this, SLOT(onPluginActivated));
       QObject::connect(info, SIGNAL(deactivated), this, SLOT(onPluginDeactivated));
       info->ukuiSettingsPluginInfoSetSchema(schema);
    } else {
           CT_SYSLOG(LOG_ERR, "Ignoring unknown module '%s'", schema);
    }
    g_free (schema);

 out:
    if (info != NULL) {
        delete info;
    }
}

static bool is_item_in_schema (const char* const* items, const char* item)
{
    while (*items) {
       if (g_strcmp0 (*items++, item) == 0) return true;
    }
    return false;
}

bool is_schema (const char* schema)
{
    return is_item_in_schema (g_settings_list_schemas(), schema);
}
