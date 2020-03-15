#include "plugin-info.h"
#include "global.h"
#include "clib-syslog.h"

#include <QDebug>
#include <QFile>

PluginInfo::PluginInfo(QString& fileName)
{
    int         priority;
    char*       str = NULL;
    GError*     error = NULL;
    GKeyFile*   pluginFile = NULL;

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
        CT_SYSLOG(LOG_ERR, "Bad plugin file:'%s', error:'%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
        g_object_unref(pluginFile);
        return;
    }
    if (!g_key_file_has_key(pluginFile, PLUGIN_GROUP, "IAge", &error)) {
        CT_SYSLOG(LOG_ERR, "IAge key does not exist in file: %s, error: '%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
    }
    /* Check IAge=2 */
    if (g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "IAge", &error) != 0) {
        CT_SYSLOG(LOG_ERR, "Wrong IAge in file: %s, error: '%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
    }

    /* Get Location */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Module", &error);
    if ((str != NULL) && (*str != '\0')) {
        mLocation = str;
    } else {
        g_free (str);
        CT_SYSLOG(LOG_ERR, "Could not find 'Module' in %s, error: '%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
    }
    /* Get Name */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Name", NULL, &error);
    if (str != NULL) {
        mName = str;
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find %s, error '%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
    }

    /* Get Description */
    str = g_key_file_get_locale_string (pluginFile, PLUGIN_GROUP, "Description", NULL, &error);
    if (str != NULL) {
        mDesc = QString(str);
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Description' in %s, error: '%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
    }

    /* Get Authors */
    mAuthors = new QList<QString>();
    char** author = g_key_file_get_string_list (pluginFile, PLUGIN_GROUP, "Authors", NULL, &error);
    if (nullptr != author) {
        for (int i = 0; author[i] != NULL; ++i) mAuthors->append(author[i]);
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Authors' in %s, error: '%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
    }
    g_strfreev (author);
    author = nullptr;

    /* Get Copyright */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Copyright", &error);
    if (str != NULL) {
        mCopyright = str;
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Copyright' in %s, error: '%s'", fileName.toUtf8().data(), error->message);
        g_object_unref(error);
        error = nullptr;
    }

    /* Get Website */
    str = g_key_file_get_string (pluginFile, PLUGIN_GROUP, "Website", &error);
    if (str != NULL) {
         mWebsite = str;
    } else {
        CT_SYSLOG(LOG_ERR, "Could not find 'Website' in %s, error: '%s'", fileName.toUtf8().data(), error->message);

        g_object_unref(error);
        error = nullptr;
    }

    /* Get Priority */
    priority = g_key_file_get_integer (pluginFile, PLUGIN_GROUP, "Priority", NULL);
    if (priority >= PLUGIN_PRIORITY_MAX) {
         this->mPriority = priority;
    } else {
         this->mPriority = PLUGIN_PRIORITY_DEFAULT;
    }

    if (nullptr != error) g_object_unref(error);
    if (nullptr != pluginFile) g_key_file_free (pluginFile);
}

PluginInfo::~PluginInfo()
{
    if (nullptr != mModule)   {delete mModule; mModule = nullptr;}
    if (nullptr != mPlugin)   {delete mPlugin; mPlugin = nullptr;}
    if (nullptr != mAuthors)  {delete mAuthors; mAuthors = nullptr;}
    if (nullptr != mSettings) {delete mSettings; mSettings = nullptr;}
}

bool PluginInfo::pluginActivate()
{
    if (!mAvailable) {CT_SYSLOG(LOG_DEBUG, "plugin is not available!") return false;}
    if (mActive) {CT_SYSLOG(LOG_DEBUG, "plugin has activity!") return true;}

    if (activatePlugin ()) {
        mActive = true;
        return true;
    }

    return false;
}

bool PluginInfo::pluginDeactivate()
{
    if (!mActive || !mAvailable) {
        return TRUE;
    }
    deactivatePlugin();
    this->mActive = FALSE;

    return TRUE;
}

bool PluginInfo::pluginIsactivate()
{
    return (mAvailable && mActive);
}

bool PluginInfo::pluginEnabled()
{
    return (this->mEnabled);
}

bool PluginInfo::pluginIsAvailable()
{
    return this->mAvailable;
}

QString& PluginInfo::getPluginName()
{
    return this->mName;
}

QString& PluginInfo::getPluginDescription()
{
    return this->mDesc;
}

QList<QString>& PluginInfo::getPluginAuthors()
{
    return *mAuthors;
}

QString& PluginInfo::getPluginWebsite()
{
    return this->mWebsite;
}

QString& PluginInfo::getPluginCopyright()
{
    return this->mCopyright;
}

QString& PluginInfo::getPluginLocation()
{
    return this->mLocation;
}

int& PluginInfo::getPluginPriority()
{
    return this->mPriority;
}

void PluginInfo::setPluginPriority(int priority)
{
    this->mPriority = priority;
}

// FIXME:// ?????
void PluginInfo::setPluginSchema(QString& schema)
{
    int priority;

    mSettings = new QGSettings(schema.toUtf8());

    this->mEnabled = mSettings->get("active").toBool();
    priority = mSettings->get("priority").toInt();
    if (priority > 0) this->mPriority = priority;
    if (!connect(mSettings, SIGNAL(changed(QString)), this, SLOT(pluginSchemaSlot(QString)))){
        CT_SYSLOG(LOG_ERR, "plugin setting '%s', connect error!", schema.toUtf8().data());
    }
}

bool PluginInfo::operator==(PluginInfo& oth)
{
    return (0 == QString::compare(mName, oth.getPluginName(), Qt::CaseInsensitive));
}

void PluginInfo::pluginSchemaSlot(QString key)
{
    // if configure has changed, modify PluginInfo
    // if configure deactivity plugin, activate() else deactivate()
}

bool PluginInfo::activatePlugin()
{
    bool res = true;

    if (!mAvailable) {
        CT_SYSLOG(LOG_ERR, "plugin is not available");
        return false;
    }

    // load module
    if (nullptr == mPlugin) {
        res = loadPluginModule();
    }

    if (res && (nullptr != mPlugin)) {
        try {
            mPlugin->activate();
        } catch (std::exception ex) {
            res = false;
            CT_SYSLOG(LOG_ERR, "plugin '%s' running error: '%s'", mName.toUtf8().data(), ex.what());
        }

    } else {
        CT_SYSLOG(LOG_ERR, "Error activating plugin '%s'", this->mName.toUtf8().data());
    }

    return res;
}

bool PluginInfo::loadPluginModule()
{
    QString     path;

    if (mFile.isNull() || mFile.isEmpty()) {CT_SYSLOG(LOG_ERR, "Plugin file is error"); return false;}
    if (mLocation.isNull() || mLocation.isEmpty()) {CT_SYSLOG(LOG_ERR, "Plugin location is error"); return false;}
    if (!mAvailable) {CT_SYSLOG(LOG_ERR, "Plugin is not available"); return false;}

    QFile file(mFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QStringList l = mFile.split("/");
    l.pop_back();
    path = l.join("/") + "/lib" + mLocation + ".so";

    if (path.isEmpty() || path.isNull()) {CT_SYSLOG(LOG_ERR, "error module path:'%s'", path.toUtf8().data()); return false;}

    mModule = new QLibrary(path);
    if (!(mModule->load())) {
        CT_SYSLOG(LOG_ERR, "create module '%s' error:'%s'", path.toUtf8().data(), mModule->errorString().toUtf8().data());
        mAvailable = false;
        return false;
    }
    typedef PluginInterface* (*createPlugin) ();
    createPlugin p = (createPlugin)mModule->resolve("createSettingsPlugin");
    if (!p) {
        CT_SYSLOG(LOG_ERR, "create module class failed, error: '%s'", mModule->errorString().toUtf8().data());
        return false;
    }
    try {
        // INFO: create plugin maybe error, plugin must check it and throw error;
        mPlugin = (PluginInterface*)p();
    } catch (std::exception ex) {
        CT_SYSLOG(LOG_ERR, "error: %s", ex.what());
    }

    return true;
}

void PluginInfo::deactivatePlugin()
{
    if (nullptr != mPlugin) {
        mPlugin->deactivate();
    }
}
