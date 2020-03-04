#ifndef UKUISETTINGSMANAGER_H
#define UKUISETTINGSMANAGER_H

#include "global.h"
#include "ukuisettingsplugininfo.h"

#include <glib.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <QList>
#include <QString>
#include <QObject>

class UkuiSettingsManager : QObject
{
    Q_OBJECT
private:
    UkuiSettingsManager();
    UkuiSettingsManager(UkuiSettingsManager&)=delete;
    UkuiSettingsManager& operator= (const UkuiSettingsManager&)=delete;

public:
    ~UkuiSettingsManager();
    static UkuiSettingsManager* ukuiSettingsManagerNew();   // DD-OK!

    // ukui_settings_manager_start
    gboolean ukuiSettingsManagerStart (GError **error);     // DD-OK!

    // ukui_settings_manager_stop
    void ukuiSettingsManagerStop ();

    // ukui_settings_manager_awake
    gboolean ukuiSettingsManagerAwake ();

signals:
    void pluginActivated (QString& name);
    void pluginDeactivated (QString& name);

public slots:
    void onPluginActivated (QString& name);
    void onPluginDeactivated (QString& name);

private:
    static gboolean registerManager();
    void loadAll ();
    void loadDir (QString& path);
    void loadFile (QString& fileName);

private:
    static QList<UkuiSettingsPluginInfo*>* mPlugin;
    static UkuiSettingsManager* mUkuiSettingsManager;
    static DBusGConnection*     mConnection;
};

#endif // UKUISETTINGSMANAGER_H
