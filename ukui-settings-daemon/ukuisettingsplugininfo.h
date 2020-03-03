#ifndef UKUISETTINGSPLUGININFO_H
#define UKUISETTINGSPLUGININFO_H
#include "ukuisettingsplugin.h"

#include <glib-object.h>
#include <gmodule.h>
#include <gio/gio.h>

class UkuiSettingsPluginInfo
{
public:
    UkuiSettingsPluginInfo();

    UkuiSettingsPluginInfo *ukuiSettingsPluginInfoNewFromFile (const char *filename);
    gboolean         ukui_settings_plugin_info_activate        (UkuiSettingsPluginInfo *info);
    gboolean         ukui_settings_plugin_info_deactivate      (UkuiSettingsPluginInfo *info);

    gboolean         ukui_settings_plugin_info_is_active       (UkuiSettingsPluginInfo *info);
    gboolean         ukui_settings_plugin_info_get_enabled     (UkuiSettingsPluginInfo *info);
    gboolean         ukui_settings_plugin_info_is_available    (UkuiSettingsPluginInfo *info);
    const char      *ukui_settings_plugin_info_get_name        (UkuiSettingsPluginInfo *info);
    const char      *ukui_settings_plugin_info_get_description (UkuiSettingsPluginInfo *info);
    const char     **ukui_settings_plugin_info_get_authors     (UkuiSettingsPluginInfo *info);
    const char      *ukui_settings_plugin_info_get_website     (UkuiSettingsPluginInfo *info);
    const char      *ukui_settings_plugin_info_get_copyright   (UkuiSettingsPluginInfo *info);
    const char      *ukui_settings_plugin_info_get_location    (UkuiSettingsPluginInfo *info);
    void             ukui_settings_plugin_info_set_schema      (UkuiSettingsPluginInfo *info,
                                                                gchar                  *schema);


public:
    enum {
        ACTIVATED,
        DEACTIVATED,
        LAST_SIGNAL
    };

private:
    char                    *file;
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
