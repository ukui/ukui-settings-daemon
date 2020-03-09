#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "global.h"
#include "plugin-info.h"

#include <glib.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <QList>
#include <QString>
#include <QObject>

namespace UkuiSettingsDaemon {
class PluginManager;
}

class PluginManager : QObject
{
    Q_OBJECT
private:
    PluginManager();
    PluginManager(PluginManager&)=delete;
    PluginManager& operator= (const PluginManager&)=delete;
    static gboolean registerManager();
    void loadAll ();
    void loadDir (QString& path);
    void loadFile (QString& fileName);
    void unloadAll ();

public:
    ~PluginManager();
    static PluginManager* getInstance();    // DD-OK!

    // ukui_settings_manager_start
    bool managerStart ();                   // DD-OK!

    // ukui_settings_manager_stop
    void managerStop ();                    // DD-OK!

    // ukui_settings_manager_awake
    bool managerAwake ();                   // DO-OK!

signals:
    void pluginActivated (QString& name);
    void pluginDeactivated (QString& name);

public slots:
    void onPluginActivated (QString& name);
    void onPluginDeactivated (QString& name);

private:
    static QList<PluginInfo*>*  mPlugin;
    static PluginManager*       mPluginManager;
    static DBusGConnection*     mConnection;
};

#endif // PLUGIN_MANAGER_H
