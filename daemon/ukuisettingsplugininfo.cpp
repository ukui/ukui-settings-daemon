#include "ukuisettingsplugininfo.h"
#include "global.h"
#include "clib_syslog.h"


UkuiSettingsPluginInfo::UkuiSettingsPluginInfo()
{

}

UkuiSettingsPluginInfo::UkuiSettingsPluginInfo(QString &fileName)
{
    GKeyFile*   pluginFile = NULL;
    char*       str = NULL;
    int         priority;
    GError*     error = NULL;

    //FIXME://
    this->mFile = QString(fileName);

    pluginFile = g_key_file_new();
    if (!g_key_file_load_from_file(pluginFile, (char*)fileName.toLatin1().data(), G_KEY_FILE_NONE, &error)) {
        CT_SYSLOG(LOG_ERR, "Bad plugin file:'%s',error:'%s'", fileName.toLatin1().data(), error->message);
        goto out;
    }
    if (!g_key_file_has_key(pluginFile, PLUGIN_GROUP, "IAge", NULL)) {
        CT_SYSLOG(LOG_ERR, "IAge key does not exist in file: %s", fileName.toLatin1().data());
        goto out;
    }
    /* Check IAge=2 */
    if (g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "IAge", NULL) != 0) {
        CT_SYSLOG(LOG_ERR, "Wrong IAge in file: %s", fileName.toLatin1().data());
        goto out;
    }

    /* Get Location */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Module", NULL);
    if ((str != NULL) && (*str != '\0')) {
        this->mLocation = str;
    } else {
        g_free (str);
        CT_SYSLOG(LOG_ERR, "Could not find 'Module' in %s", fileName.toLatin1().data());
        goto out;
    }
    /* Get Name */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Name", NULL, NULL);
    if (str != NULL) {
        this->mName = str;
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Name' in %s", fileName.toLatin1().data());
        goto out;
    }

    /* Get Description */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Description", NULL, NULL);
    if (str != NULL) {
        this->mDesc = str;
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Description' in %s", fileName.toLatin1().data());
    }

    /* Get Authors */
    this->authors = g_key_file_get_string_list (pluginFile, PLUGIN_GROUP, "Authors", NULL, NULL);
    if (this->authors == NULL) {
        CT_SYSLOG(LOG_ERR, "Could not find 'Authors' in %s", fileName.toLatin1().data());
    }

    /* Get Copyright */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Copyright", NULL);
    if (str != NULL) {
        this->mCopyright = str;
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Copyright' in %s", fileName.toLatin1().data());
    }

    /* Get Website */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Website", NULL);
    if (str != NULL) {
         this->mWebsite = str;
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Website' in %s", fileName.toLatin1().data());
    }

    /* Get Priority */
    priority = g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "Priority", NULL);
    if (priority >= PLUGIN_PRIORITY_MAX) {
         this->mPriority = priority;
    } else {
         this->mPriority = PLUGIN_PRIORITY_DEFAULT;
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

QString& UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetName()
{
    return this->mName;
}

QString& UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetDescription()
{
    return this->mDesc;
}

const char **UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetAuthors()
{
    return (const char**)this->authors;
}

QString& UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetWebsite()
{
    return this->mWebsite;
}

QString& UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetCopyright()
{
    return this->mCopyright;
}

QString& UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetLocation()
{
    return this->mLocation;
}

int& UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetPriority()
{
    return this->mPriority;
}

void UkuiSettingsPluginInfo::ukuiSettingsPluginInfoSetPriority(int priority)
{
    this->mPriority = priority;
}

void UkuiSettingsPluginInfo::ukuiSettingsPluginInfoSetSchema(QString& schema)
{
    int priority;
    this->settings = g_settings_new(schema.toLatin1().data());
    this->enabled = g_settings_get_boolean(this->settings, "active");
    priority = g_settings_get_int(this->settings, "priority");
    if (priority > 0) this->mPriority = priority;
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
        g_warning("Error activating plugin '%s'", this->mName.toUtf8().data());
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
//    g_return_val_if_fail (this->mFile != NULL, FALSE);
//    g_return_val_if_fail (this->location != NULL, FALSE);
    g_return_val_if_fail (this->plugin == NULL, FALSE);
    g_return_val_if_fail (this->available, FALSE);
    // ukui_settings_profile_start ("%s", info->priv->location);

    dirname = g_path_get_dirname((char*)this->mFile.toUtf8().data());
    if (NULL == dirname) return FALSE;

    // FIXME://

//    path = g_module_build_path(dirname, this->location);
//    g_free(dirname);
    g_return_val_if_fail(NULL != path, FALSE);

    // 新建模块
//    this->module = ;
    g_free(path);

    if (!g_type_module_use(this->module)) {
        CT_SYSLOG(LOG_ERR, "Cannot load plugin '%s' since file '%s' cannot be read.",
                  this->mName.toUtf8().data(), NULL/*ukui_settings_module_get_path (UKUI_SETTINGS_MODULE (info->priv->module))*/);
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
