#include "ukuisettingsplugininfo.h"
#include "global.h"
#include "clib_syslog.h"


//UkuiSettingsPluginInfo::UkuiSettingsPluginInfo()
//{

//}

UkuiSettingsPluginInfo::UkuiSettingsPluginInfo(QString& fileName)
{
    GKeyFile*   pluginFile = NULL;
    char*       str = NULL;
    int         priority;
    GError*     error = NULL;

    mPriority = 0;
    mActive = false;
    mEnabled = true;
    mPlugin = nullptr;
    mModule = nullptr;
    mAvailable = true;
    mSettings = nullptr;

    QByteArray* bt = new QByteArray(fileName.toUtf8().data());
    mFile = *bt;

    pluginFile = g_key_file_new();
    if (!g_key_file_load_from_file(pluginFile, (char*)fileName.toUtf8().data(), G_KEY_FILE_NONE, &error)) {
        CT_SYSLOG(LOG_ERR, "Bad plugin file:'%s',error:'%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        goto out;
    }
    if (!g_key_file_has_key(pluginFile, PLUGIN_GROUP, "IAge", &error)) {
        CT_SYSLOG(LOG_ERR, "IAge key does not exist in file: %s", fileName.toUtf8().data());
        g_object_unref(error);
        goto out;
    }
    /* Check IAge=2 */
    if (g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "IAge", &error) != 0) {
        CT_SYSLOG(LOG_ERR, "Wrong IAge in file: %s", fileName.toUtf8().data());
        g_object_unref(error);
        goto out;
    }

    /* Get Location */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Module", &error);
    if ((str != NULL) && (*str != '\0')) {
        mLocation = str;
    } else {
        g_free (str);
        CT_SYSLOG(LOG_ERR, "Could not find 'Module' in %s", fileName.toUtf8().data());
        g_object_unref(error);
        goto out;
    }
    /* Get Name */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Name", NULL, NULL);
    if (str != NULL) {
        mName = QString(str);
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Name' in %s", fileName.toUtf8().data());
        g_object_unref(error);
        goto out;
    }

    /* Get Description */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Description", NULL, NULL);
    if (str != NULL) {
        mDesc = QString(str);
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Description' in %s", fileName.toUtf8().data());
    }

    /* Get Authors */
    this->mAuthors = g_key_file_get_string_list (pluginFile, PLUGIN_GROUP, "Authors", NULL, NULL);
    if (this->mAuthors == NULL) {
        CT_SYSLOG(LOG_ERR, "Could not find 'Authors' in %s", fileName.toUtf8().data());
    }

    /* Get Copyright */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Copyright", NULL);
    if (str != NULL) {
        mCopyright = QString(str);
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Copyright' in %s", fileName.toUtf8().data());
    }

    /* Get Website */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Website", NULL);
    if (str != NULL) {
         mWebsite = QString(str);
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Website' in %s", fileName.toUtf8().data());
    }

    /* Get Priority */
    priority = g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "Priority", NULL);
    if (priority >= PLUGIN_PRIORITY_MAX) {
         this->mPriority = priority;
    } else {
         this->mPriority = PLUGIN_PRIORITY_DEFAULT;
    }

    g_key_file_free (pluginFile);
out:
    ;
}

bool UkuiSettingsPluginInfo::ukuiSettingsPluginInfoActivate()
{
    // FIXME:// debug
//    mAvailable = true;
//    mActive = false;

    if (!mAvailable) {CT_SYSLOG(LOG_DEBUG, "plugin is not available!") return false;}
    if (mActive) {CT_SYSLOG(LOG_DEBUG, "plugin has activity!") return true;}

    if (activatePlugin ()) {
        mActive = true;
        return true;
    }

    return false;
}

bool UkuiSettingsPluginInfo::ukuiSettingsPluginInfoDeactivate()
{
//    if (!this->active || !this->available) {
//        return TRUE;
//    }
    deactivatePlugin();
    this->mActive = FALSE;

    return TRUE;
}

bool UkuiSettingsPluginInfo::ukuiSettingsPluginInfoIsactivate()
{
    return (/*this->available &&*/ this->mActive);
}

bool UkuiSettingsPluginInfo::ukuiSettingsPluginInfoGetEnabled()
{
    return (this->mEnabled);
}

bool UkuiSettingsPluginInfo::ukuiSettingsPluginInfoIsAvailable()
{
    return this->mEnabled;
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
    return (const char**)this->mAuthors;
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

    mSettings = g_settings_new(schema.toLatin1().data());
    this->mEnabled = g_settings_get_boolean(mSettings, "active");
    priority = g_settings_get_int(mSettings, "priority");
    if (priority > 0) this->mPriority = priority;
    // g_signal_connect (G_OBJECT (info->priv->settings), "changed::active",
    // G_CALLBACK (plugin_enabled_cb), info);
}

bool UkuiSettingsPluginInfo::activatePlugin()
{
    bool res = true;

    if (!mAvailable) {CT_SYSLOG(LOG_ERR, "plugin is not available"); return false;}

    // load module
    if (nullptr == mPlugin) {
        CT_SYSLOG(LOG_ERR, "start load module: '%s'", ukuiSettingsPluginInfoGetName().toUtf8().data());
        res = loadPluginModule();
    }

    if (res) {
//        ukuiSettingsPluginActivate();
        // g_signal_emit (info, signals [ACTIVATED], 0);
    } else {
        CT_SYSLOG(LOG_ERR, "Error activating plugin '%s'", this->mName.toUtf8().data());
    }

    return res;
}

bool UkuiSettingsPluginInfo::loadPluginModule()
{
    QString     path;
    char*       dirname = NULL;
    bool        ret;

    ret = false;

    if (mFile.isNull() || mFile.isEmpty()) {CT_SYSLOG(LOG_ERR, "Plugin file is error"); return false;}
    if (mLocation.isNull() || mLocation.isEmpty()) {CT_SYSLOG(LOG_ERR, "Plugin location is error"); return false;}
    if (!mAvailable) {CT_SYSLOG(LOG_ERR, "Plugin is not available"); return false;}

    dirname = g_path_get_dirname((const char*)this->mFile.toUtf8().data());
    if (NULL == dirname) return FALSE;

    QStringList l = mFile.split("/");
    l.pop_back();
    path = l.join("/") + "/lib" + mLocation;
    g_free(dirname);

    if (path.isEmpty() || path.isNull()) {CT_SYSLOG(LOG_ERR, "error module path:'%s'", path.toUtf8().data()); return false;}

    // 新建模块
    mModule = new QLibrary(path);
    if (!(mModule->load())) {
        CT_SYSLOG(LOG_ERR, "create module '%s' error!", path.toUtf8().data());
        mAvailable = false;
        return false;
    }
    typedef UkuiSettingsPlugin* (*createPlugin) ();
    createPlugin p = (createPlugin)mModule->resolve("createSettingsPlugin");
    if (!p) {
        CT_SYSLOG(LOG_ERR, "create module class failed");
        return false;
    }
    mPlugin = (UkuiSettingsPlugin*)p();

    return true;
}

// FIXME://
void UkuiSettingsPluginInfo::deactivatePlugin()
{
    ukuiSettingsPluginInfoDeactivate();
    // 发送信号 deactivate;
}

bool UkuiSettingsPluginInfo::pluginEnabledCB(GSettings *settings, gchar *key, UkuiSettingsPluginInfo*)
{
    if (g_settings_get_boolean(mSettings, key)) {
//        UkuiSettingsPluginInfoActivite();
    } else {
//        ukui_settings_plugin_info_deactivate (info);
    }
}
