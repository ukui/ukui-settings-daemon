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
#ifndef GLOBAL_H
#define GLOBAL_H
#include "clib-syslog.h"


#define PLUGIN_PRIORITY_MAX                         1
#define PLUGIN_PRIORITY_DEFAULT                     100

#define PLUGIN_GROUP                                "UKUI Settings Plugin"

#define UKUI_SETTINGS_DAEMON_DBUS_NAME              "org.ukui.SettingsDaemon"
#define UKUI_SETTINGS_DAEMON_DBUS_PATH              "/daemon/registry"

#define UKUI_SETTINGS_DAEMON_MANAGER_DBUS_PATH      "/org/ukui/SettingsDaemon"

#define USD_MANAGER_DBUS_PATH                       "/org/ukui/SettingsDaemon"
#define DEFAULT_SETTINGS_PREFIX                     "org.ukui.SettingsDaemon"
#define PLUGIN_EXT                                  ".ukui-settings-plugin"
#define UKUI_SETTINGS_PLUGINDIR                     "/usr/lib/ukui-settings-daemon"       // plugin dir

#endif // GLOBAL_H
