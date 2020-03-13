#ifndef GLOBAL_H
#define GLOBAL_H
#include "clib-syslog.h"


#define PLUGIN_PRIORITY_MAX             1
#define PLUGIN_PRIORITY_DEFAULT         100

#define PLUGIN_GROUP                    "UKUI Settings Plugin"

#define UKUI_SETTINGS_DAEMON_DBUS_NAME              "org.ukui.SettingsDaemon"
#define UKUI_SETTINGS_DAEMON_DBUS_PATH              "/daemon/registry"

#define UKUI_SETTINGS_DAEMON_MANAGER_DBUS_PATH      "/org/ukui/SettingsDaemon"

#define USD_MANAGER_DBUS_PATH           "/org/ukui/SettingsDaemon"
#define DEFAULT_SETTINGS_PREFIX         "org.ukui.SettingsDaemon"
#define PLUGIN_EXT                      ".ukui-settings-plugin"
#define UKUI_SETTINGS_PLUGINDIR         "/usr/local/lib/ukui-settings-daemon"       // plugin dir

#endif // GLOBAL_H
