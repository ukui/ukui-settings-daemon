#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "global.h"
#include "plugin-info.h"

#include <QList>
#include <QString>
#include <QObject>
#include <QDBusConnection>

namespace UkuiSettingsDaemon {
class PluginManager;
}

class PluginManager : QObject
{
    Q_OBJECT
    Q_CLASSINFO ("D-Bus Interface", UKUI_SETTINGS_DAEMON_DBUS_NAME)

public:
    ~PluginManager();
    static PluginManager* getInstance();

private:
    PluginManager();
    PluginManager(PluginManager&)=delete;
    PluginManager& operator= (const PluginManager&)=delete;

    void loadAll ();
    void loadDir (QString& path);
    void loadFile (QString& fileName);
    void unloadAll ();

public Q_SLOTS:
    bool managerStart ();
    void managerStop ();
    bool managerAwake ();

private:
    QList<PluginInfo*>*         mPlugin;
    static PluginManager*       mPluginManager;
};

#endif // PLUGIN_MANAGER_H
