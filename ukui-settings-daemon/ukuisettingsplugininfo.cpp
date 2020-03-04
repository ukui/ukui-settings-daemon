#include "ukuisettingsplugininfo.h"

#define PLUGIN_GROUP "UKUI Settings Plugin"
#define PLUGIN_PRIORITY_MAX 1
#define PLUGIN_PRIORITY_DEFAULT 100


UkuiSettingsPluginInfo::UkuiSettingsPluginInfo()
{

}

UkuiSettingsPluginInfo::UkuiSettingsPluginInfo(QString &fileName)
{
    GKeyFile*   pluginFile = NULL;
    char*       str = NULL;
    int         priority;
    gboolean    ret;

    // ukui_settings_profile_start ("%s", filename);
    ret = FALSE;

    this->file = QString(fileName);

    pluginFile = g_key_file_new();
    if (!g_key_file_load_from_file(pluginFile, (char*)fileName.data(), G_KEY_FILE_NONE, NULL)) {
        g_warning("Bad plugin file: %s", fileName.data());
        goto out;
    }
    if (!g_key_file_has_key(pluginFile, PLUGIN_GROUP, "IAge", NULL)) {
        g_debug ("IAge key does not exist in file: %s", fileName.data());
        goto out;
    }
    /* Check IAge=2 */
    if (g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "IAge", NULL) != 0) {
        g_debug ("Wrong IAge in file: %s", fileName.data());
        goto out;
    }

    /* Get Location */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Module", NULL);
    if ((str != NULL) && (*str != '\0')) {
        this->location = str;
    } else {
        g_free (str);
        g_warning ("Could not find 'Module' in %s", fileName.data());
        goto out;
    }
    /* Get Name */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Name", NULL, NULL);
    if (str != NULL) {
        this->name = str;
    } else {
        g_warning ("Could not find 'Name' in %s", fileName.data());
        goto out;
    }

    /* Get Description */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Description", NULL, NULL);
    if (str != NULL) {
        this->desc = str;
    } else {
        g_debug ("Could not find 'Description' in %s", fileName.data());
    }

    /* Get Authors */
    this->authors = g_key_file_get_string_list (pluginFile, PLUGIN_GROUP, "Authors", NULL, NULL);
    if (this->authors == NULL) {
        g_debug ("Could not find 'Authors' in %s", fileName.data());
    }

    /* Get Copyright */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Copyright", NULL);
    if (str != NULL) {
        this->copyright = str;
    } else {
        g_debug ("Could not find 'Copyright' in %s", fileName.data());
    }

    /* Get Website */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Website", NULL);
    if (str != NULL) {
         this->website = str;
    } else {
        g_debug ("Could not find 'Website' in %s", fileName.data());
    }

    /* Get Priority */
    priority = g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "Priority", NULL);
    if (priority >= PLUGIN_PRIORITY_MAX) {
         this->priority = priority;
    } else {
         this->priority = PLUGIN_PRIORITY_DEFAULT;
    }

    g_key_file_free (pluginFile);

//    debug_info (info);
    this->available = TRUE;

out:
    // ukui_settings_profile_end ("%s", filename);
    ;
}

gboolean UkuiSettingsPluginInfo::ukuiSettingsPluginInfoActivate()
{
    if (! this->available) {
        return FALSE;
    }

    if (this->active) {
        return TRUE;
    }

    if (activatePlugin ()) {
        this->active = TRUE;
        return TRUE;
    }

    return FALSE;
}

gboolean UkuiSettingsPluginInfo::ukuiSettingsPluginInfoDeactivate()
{
    if (!this->active || !this->available) {
        return TRUE;
    }
    deactivatePlugin();
    this->active = FALSE;

    return TRUE;
}

gboolean UkuiSettingsPluginInfo::ukuiSettingsPluginInfoIsactivate()
{
    return (this->available && this->active);
}

gboolean UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetEnabled()
{
    return (this->enabled);
}

gboolean UkuiSettingsPluginInfo::ukuiSettingsPluginInfoIsAvailable()
{
    return this->enabled;
}

const char* UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetName()
{
    return this->name;
}

const char *UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetDescription()
{
    return this->desc;
}

const char **UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetAuthors()
{
    return (const char**)this->authors;
}

const char *UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetWebsite()
{
    return this->website;
}

const char *UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetCopyright()
{
    return this->copyright;
}

const char *UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetLocation()
{
    return this->location;
}

int UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetPriority()
{
    return this->priority;
}

void UkuiSettingsPluginInfo::ukuiSettingsPluginInfoSetPriority(int priority)
{
    this->priority = priority;
}

void UkuiSettingsPluginInfo::ukuiSettingsPluginInfoSetSchema(gchar *schema)
{
    int priority;
    this->settings = g_settings_new(schema);
    this->enabled = g_settings_get_boolean(this->settings, "active");
    priority = g_settings_get_int(this->settings, "priority");
    if (priority > 0) this->priority = priority;
    // g_signal_connect (G_OBJECT (info->priv->settings), "changed::active",
    // G_CALLBACK (plugin_enabled_cb), info);
}

gboolean UkuiSettingsPluginInfo::activatePlugin()
{
    gboolean res = TRUE;
    if (!this->available) {
        return FALSE;
    }
    if (NULL == this->plugin) {
        res = loadPluginModule();
    }
    if (res) {
        //ukuiSettingsPluginActivate();
        // g_signal_emit (info, signals [ACTIVATED], 0);
    } else {
        g_warning("Error activating plugin '%s'", this->name);
    }

    return res;
}

gboolean UkuiSettingsPluginInfo::loadPluginModule()
{
    char*       path = NULL;
    char*       dirname = NULL;
    gboolean    ret;

    ret = FALSE;

//    g_return_val_if_fail (UKUI_IS_SETTINGS_PLUGIN_INFO (info), FALSE);
    g_return_val_if_fail (this->file != NULL, FALSE);
    g_return_val_if_fail (this->location != NULL, FALSE);
    g_return_val_if_fail (this->plugin == NULL, FALSE);
    g_return_val_if_fail (this->available, FALSE);
    // ukui_settings_profile_start ("%s", info->priv->location);

    dirname = g_path_get_dirname((char*)this->file.data());
    if (NULL == dirname) return FALSE;

    // FIXME://

//    path = g_module_build_path(dirname, this->location);
//    g_free(dirname);
    g_return_val_if_fail(NULL != path, FALSE);

    // 新建模块
//    this->module = ;
    g_free(path);

    if (!g_type_module_use(this->module)) {
        g_warning("Cannot load plugin '%s' since file '%s' cannot be read.",
                  this->name, NULL/*ukui_settings_module_get_path (UKUI_SETTINGS_MODULE (info->priv->module))*/);
        //模块
//        this->module = NULL;
        this->available = FALSE;
        goto out;
    }
//    this->plugin = UKUI_SETTINGS_PLUGIN (ukui_settings_module_new_object (UKUI_SETTINGS_MODULE (this->module)));
//    g_type_module_unuse (this->module);
    ret = TRUE;

out:
    // ukui_settings_profile_end ("%s", info->priv->location);
    ;
    return ret;

}

// FIXME://
void UkuiSettingsPluginInfo::deactivatePlugin()
{
    ukuiSettingsPluginInfoDeactivate();
    // 发送信号 deactivate;
}

gboolean UkuiSettingsPluginInfo::pluginEnabledCB(GSettings *settings, gchar *key, UkuiSettingsPluginInfo*)
{
    if (g_settings_get_boolean(this->settings, key)) {
//        UkuiSettingsPluginInfoActivite();
    } else {
//        ukui_settings_plugin_info_deactivate (info);
    }
}
