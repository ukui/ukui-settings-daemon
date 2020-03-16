#include "plugin-manager.h"

#include "global.h"
#include "clib-syslog.h"
#include "plugin-info.h"

#include <glib.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gio/gio.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <QDebug>
#include <QDBusError>
#include <QDBusConnectionInterface>

QList<PluginInfo*>* PluginManager::mPlugin = nullptr;
PluginManager* PluginManager::mPluginManager = nullptr;

static bool is_schema (QString& schema);
static bool register_manager(PluginManager& pm);

PluginManager::PluginManager()
{
    if (nullptr == mPlugin) mPlugin = new QList<PluginInfo*>();
}

PluginManager::~PluginManager()
{
    managerStop();
    delete mPlugin;
    mPlugin = nullptr;
}

PluginManager* PluginManager::getInstance()
{
    if (nullptr == mPluginManager) {
        CT_SYSLOG(LOG_DEBUG, "ukui settings manager will be created!")
        mPluginManager = new PluginManager;
        if (!register_manager(*mPluginManager)) {
            return nullptr;
        }
    }

    return mPluginManager;
}

bool PluginManager::managerStart()
{
    GDir*                   dir = NULL;
    QString                 schema;
    GError*                 error = NULL;
    const char*             name = NULL;

    CT_SYSLOG(LOG_DEBUG, "Starting settings manager");

    QString path(UKUI_SETTINGS_PLUGINDIR);
    dir = g_dir_open ((char*)path.toUtf8().data(), 0, &error);
    if (NULL == dir) {
        CT_SYSLOG(LOG_ERR, "%s", error->message);
        g_error_free(error);
        error = nullptr;
        return false;
    }

    while ((name = g_dir_read_name(dir))) {
        char* filename = NULL;
        if (!g_str_has_suffix(name, PLUGIN_EXT)) {
            continue;
        }
        filename = g_build_filename((char*)path.toUtf8().data(), name, NULL);
        if (g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
            QString ftmp(filename);
            PluginInfo* info = new PluginInfo(ftmp);
            if (info == NULL) {
                continue;
            }

            if (mPlugin->contains(info)) {
                CT_SYSLOG(LOG_DEBUG, "The list has contain this plugin, '%s'", ftmp.toUtf8().data());
                if (info != NULL) delete info;
            }

            // check plugin's schema
            schema = QString("%1.plugins.%2").arg(DEFAULT_SETTINGS_PREFIX).arg(info->getPluginLocation().toUtf8().data());
            if (is_schema (schema)) {
                CT_SYSLOG(LOG_DEBUG, "right schema '%s'", schema.toUtf8().data());
                info->setPluginSchema(schema);
                mPlugin->insert(0, info);
            } else {
                CT_SYSLOG(LOG_ERR, "Ignoring unknown schema '%s'", schema.toUtf8().data());
                if (info != NULL) delete info;
            }
        }
        g_free(filename);
    }
    g_dir_close(dir);

    // FIXME:// sort plugin

    CT_SYSLOG(LOG_DEBUG, "Now Activity plugins ...");
    for (int i = 0; i < mPlugin->size(); ++i) {
        PluginInfo* info = mPlugin->at(i);
        CT_SYSLOG(LOG_DEBUG, "start activity plugin: %s ...", info->getPluginName().toUtf8().data());
        info->pluginActivate();
    }

    return true;
}

void PluginManager::managerStop()
{
    CT_SYSLOG(LOG_DEBUG, "Stopping settings manager");
    while (!mPlugin->isEmpty()) {
        PluginInfo* plugin = mPlugin->takeFirst();
        plugin->pluginDeactivate();
        delete plugin;
    }

    // exit main event loop
    QCoreApplication::exit();
}

bool PluginManager::managerAwake()
{
    CT_SYSLOG(LOG_DEBUG, "Awake called")
    return managerStart();
}

static bool is_item_in_schema (const char* const* items, QString& item)
{
    while (*items) {
       if (g_strcmp0 (*items++, item.toLatin1().data()) == 0) return true;
    }
    return false;
}

bool is_schema (QString& schema)
{
    return is_item_in_schema (g_settings_list_schemas(), schema);
}

static bool register_manager(PluginManager& pm)
{
    QString ukuiDaemonBusName = UKUI_SETTINGS_DAEMON_DBUS_NAME;

    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(ukuiDaemonBusName)) {
        return false;
    }

    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(UKUI_SETTINGS_DAEMON_DBUS_NAME)) {
        CT_SYSLOG(LOG_ERR, "error getting system bus: '%s'", bus.lastError().message().toUtf8().data());
        return false;
    }

    if (!bus.registerObject(UKUI_SETTINGS_DAEMON_DBUS_PATH, (QObject*)&pm, QDBusConnection::ExportAllSlots)) {
        CT_SYSLOG(LOG_ERR, "regist settings manager error: '%s'", bus.lastError().message().toUtf8().data());
        return false;
    }

    CT_SYSLOG(LOG_DEBUG, "regist settings manager successful!");

    return true;
}
