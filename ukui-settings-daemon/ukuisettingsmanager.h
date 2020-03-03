#ifndef UKUISETTINGSMANAGER_H
#define UKUISETTINGSMANAGER_H

#include <QString>
#include <glib.h>
#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

class UkuiSettingsManager
{
private:
    UkuiSettingsManager();
    UkuiSettingsManager(UkuiSettingsManager&)=delete;
    UkuiSettingsManager& operator= (const UkuiSettingsManager&)=delete;

    static bool register_manager();

public:
    ~UkuiSettingsManager();
    static UkuiSettingsManager* ukuiSettingsManagerNew();

    // ukui_settings_manager_start
    gboolean ukuiSettingsManagerStart (GError **error);

private:
    static gboolean registerManager();
    void loadAll ();
    void loadDir (QString& path);
    void loadFile (QString& fileName);

private:
    static UkuiSettingsManager* mUkuiSettingsManager;
    static DBusGConnection*     connection;
};

#endif // UKUISETTINGSMANAGER_H
