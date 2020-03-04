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

    const char* ukuiSettingsPluginInfoGetName ();
    const char* ukuiSettingsPluginInfoGetDescription ();
    const char** ukuiSettingsPluginInfoGetAuthors ();
    const char* ukuiSettingsPluginInfoGetWebsite ();
    const char* ukuiSettingsPluginInfoGetCopyright ();
    const char* ukuiSettingsPluginInfoGetLocation ();

    int ukuiSettingsPluginInfoGetPriority ();

    void ukuiSettingsPluginInfoSetPriority (int priority);
    void ukuiSettingsPluginInfoSetSchema (gchar* schema);

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
    QString file;

    GSettings               *settings;

    char                    *location;
    GTypeModule             *module;

    char                    *name;
    char                    *desc;
    char                   **authors;
    char                    *copyright;
    char                    *website;

    UkuiSettingsPlugin     *plugin;

    int                      enabled : 1;
    int                      active : 1;

    /* A plugin is unavailable if it is not possible to activate it
       due to an error loading the plugin module */
    int                      available : 1;

    guint                    enabled_notification_id;

    /* Priority determines the order in which plugins are started and
     * stopped. A lower number means higher priority. */
    guint                    priority;
};

#endif // UKUISETTINGSPLUGININFO_H
