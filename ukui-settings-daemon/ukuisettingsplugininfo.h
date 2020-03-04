#ifndef UKUISETTINGSPLUGININFO_H
#define UKUISETTINGSPLUGININFO_H
#include "ukuisettingsplugin.h"

#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>

#include <QObject>

class UkuiSettingsPluginInfo : public QObject
{
    Q_OBJECT
public:
    UkuiSettingsPluginInfo();
    UkuiSettingsPluginInfo(QString& fileName); // ukui_settings_plugin_info_new_from_file (const char *filename);

    gboolean ukuiSettingsPluginInfoActivate ();
    gboolean ukuiSettingsPluginInfoDeactivate ();
    gboolean ukuiSettingsPluginInfoIsactivate ();
    gboolean ukuiSettingsPluginInfoGetEnabled ();
    gboolean ukuiSettingsPluginInfoIsAvailable ();

    QString& ukuiSettingsPluginInfoGetName ();
    QString& ukuiSettingsPluginInfoGetDescription ();
    const char** ukuiSettingsPluginInfoGetAuthors ();
    QString& ukuiSettingsPluginInfoGetWebsite ();
    QString& ukuiSettingsPluginInfoGetCopyright ();
    QString& ukuiSettingsPluginInfoGetLocation ();

    int& ukuiSettingsPluginInfoGetPriority ();

    void ukuiSettingsPluginInfoSetPriority (int priority);
    void ukuiSettingsPluginInfoSetSchema (QString& schema);

    GType ukuiSettingsPluginInfoGetType (void) G_GNUC_CONST;

signals:
    void activated(QString&);
    void deactivated(QString&);

private:
    gboolean activatePlugin();
    gboolean loadPluginModule();
    void deactivatePlugin();
    gboolean pluginEnabledCB(GSettings* settings, gchar* key, UkuiSettingsPluginInfo*);

private:
    /* Priority determines the order in which plugins are started and stopped. A lower number means higher priority. */
    int                     mPriority;
    QString                 mFile;
    QString                 mLocation;
    QString                 mName;
    QString                 mDesc;
    QString                 mCopyright;
    QString                 mWebsite;
    GSettings               *settings;
    char                   **authors;

    GTypeModule             *module;

    UkuiSettingsPlugin     *plugin;

    int                      enabled : 1;
    int                      active : 1;

    /* A plugin is unavailable if it is not possible to activate it
       due to an error loading the plugin module */
    int                      available : 1;

    guint                    enabled_notification_id;


};

#endif // UKUISETTINGSPLUGININFO_H
