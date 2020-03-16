#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "global.h"
#include "plugin-info.h"

#include <QList>
#include <QString>
#include <QObject>
#include <QApplication>
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

Q_SIGNALS:
    void exit ();

public Q_SLOTS:
    void managerStop ();
    bool managerStart ();
    bool managerAwake ();

private:
    static QList<PluginInfo*>*      mPlugin;
    static PluginManager*           mPluginManager;
};

#endif // PLUGIN_MANAGER_H
