/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "plugin-manager.h"
#include <QStandardPaths>
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
bool sortPluginByPriority(PluginInfo* a,PluginInfo* b);

PluginManager::PluginManager()
{
    if (nullptr == mPlugin) mPlugin = new QList<PluginInfo*>();
}

PluginManager::~PluginManager()
{
    //managerStop();
    //delete mPlugin;
    //mPlugin = nullptr;
}

PluginManager* PluginManager::getInstance()
{
    if (nullptr == mPluginManager) {
        USD_LOG(LOG_DEBUG, "ukui settings manager will be created!")
        mPluginManager = new PluginManager;
        if (!register_manager(*mPluginManager)) {
            USD_LOG(LOG_ERR, "register manager failed!");
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

    qDebug("Starting settings manager");

    QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    QString libpath = qApp->libraryPaths().at(0);
    QString path = libpath.mid(0,libpath.lastIndexOf('/')-4)+"/ukui-settings-daemon";

    dir = g_dir_open ((char*)path.toUtf8().data(), 0, &error);
    if (NULL == dir) {
        USD_LOG(LOG_ERR, "%s", error->message);
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
                USD_LOG(LOG_DEBUG, "The list has contain this plugin, '%s'", ftmp.toUtf8().data());
                if (info != NULL) delete info;
            }

            // check plugin's schema
            schema = QString("%1.plugins.%2").arg(DEFAULT_SETTINGS_PREFIX).arg(info->getPluginLocation().toUtf8().data());
            if (is_schema (schema)) {
                USD_LOG(LOG_DEBUG, "right schema '%s'", schema.toUtf8().data());
                info->setPluginSchema(schema);
                mPlugin->insert(0, info);
            } else {
                USD_LOG(LOG_ERR, "Ignoring unknown schema '%s'", schema.toUtf8().data());
                if (info != NULL) delete info;
            }
        }
        g_free(filename);
    }
    g_dir_close(dir);

    //sort plugin
	qSort(mPlugin->begin(),mPlugin->end(),sortPluginByPriority);

    USD_LOG(LOG_DEBUG, "Now Activity plugins ...");
    for (int i = 0; i < mPlugin->size(); ++i) {
        PluginInfo* info = mPlugin->at(i);
        USD_LOG(LOG_DEBUG, "start activity plugin: %s ...", info->getPluginName().toUtf8().data());
        info->pluginActivate();
    }
    USD_LOG(LOG_DEBUG, "All plugins has been activited!");

    return true;
}

void PluginManager::managerStop()
{
    USD_LOG(LOG_DEBUG, "Stopping settings manager");

    while (!mPlugin->isEmpty()) {
        PluginInfo* plugin = mPlugin->takeFirst();
        USD_LOG(LOG_DEBUG, "start Daectivity plugin: %s ...", plugin->getPluginName().toUtf8().data());
        plugin->pluginDeactivate();
        USD_LOG(LOG_DEBUG, "Daectivity plugin: %s ...ok", plugin->getPluginName().toUtf8().data());
        //delete plugin;
    }

    USD_LOG(LOG_DEBUG,"Daectivity all plugin over..");
    // exit main event loop
    QApplication::exit(0);
}

bool PluginManager::managerAwake()
{
    USD_LOG(LOG_DEBUG, "Awake called")
    return managerStart();
}

static bool is_item_in_schema (const gchar* const* items, QString& item)
{
    while (*items) {
       if (g_strcmp0 (*items++, item.toLatin1().data()) == 0) {
           return true;
       }
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
        USD_LOG(LOG_ERR, "error getting system bus: '%s'", bus.lastError().message().toUtf8().data());
        return false;
    }

    if (!bus.registerObject(UKUI_SETTINGS_DAEMON_DBUS_PATH, (QObject*)&pm, QDBusConnection::ExportAllSlots)) {
        USD_LOG(LOG_ERR, "regist settings manager error: '%s'", bus.lastError().message().toUtf8().data());
        return false;
    }

    USD_LOG(LOG_DEBUG, "regist settings manager successful!");

    return true;
}

bool sortPluginByPriority(PluginInfo* a,PluginInfo* b)
{
    return a->getPluginPriority() < b->getPluginPriority();
}
