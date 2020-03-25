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
    if (nullptr != mModule)   {mModule->unload(); delete mModule; mModule = nullptr;}
    if (nullptr != mAuthors)  {delete mAuthors; mAuthors = nullptr;}
    if (nullptr != mSettings) {delete mSettings; mSettings = nullptr;}
}

bool PluginInfo::pluginActivate()
{
    bool res = false;

    if (!mAvailable) {CT_SYSLOG(LOG_DEBUG, "plugin is not available!") return false;}
    if (mActive) {CT_SYSLOG(LOG_DEBUG, "plugin has activity!") return true;}

    // load module
    if (nullptr == mPlugin) {
        res = loadPluginModule(*this);
    }

    if (res && (nullptr != mPlugin)) {
        mPlugin->activate();
        mActive = true;
        res = true;
    } else {
        res = false;
        CT_SYSLOG(LOG_ERR, "Error activating plugin '%s'", this->mName.toUtf8().data());
    }

    return res;
}

bool PluginInfo::pluginDeactivate()
{
    if (!mActive || !mAvailable) {
        return true;
    }

    if (nullptr != mPlugin) {
        mPlugin->deactivate();
    } else {
        return false;
    }

    mActive = false;
    return true;
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

int PluginInfo::getPluginPriority()
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

void PluginInfo::pluginSchemaSlot(QString)
{
    // if configure has changed, modify PluginInfo
    // if configure deactivity plugin, activate() else deactivate()
}

bool loadPluginModule(PluginInfo& pinfo)
{
    QString     path;

    if (pinfo.mFile.isNull() || pinfo.mFile.isEmpty()) {CT_SYSLOG(LOG_ERR, "Plugin file is error"); return false;}
    if (pinfo.mLocation.isNull() || pinfo.mLocation.isEmpty()) {CT_SYSLOG(LOG_ERR, "Plugin location is error"); return false;}
    if (!pinfo.mAvailable) {CT_SYSLOG(LOG_ERR, "Plugin is not available"); return false;}

    QFile file(pinfo.mFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QStringList l = pinfo.mFile.split("/");
    l.pop_back();
    path = l.join("/") + "/lib" + pinfo.mLocation + ".so";

    if (path.isEmpty() || path.isNull()) {CT_SYSLOG(LOG_ERR, "error module path:'%s'", path.toUtf8().data()); return false;}

    pinfo.mModule = new QLibrary(path);
    pinfo.mModule->setLoadHints(QLibrary::ResolveAllSymbolsHint | QLibrary::ExportExternalSymbolsHint);
    if (!(pinfo.mModule->load())) {
        CT_SYSLOG(LOG_ERR, "create module '%s' error:'%s'", path.toUtf8().data(), pinfo.mModule->errorString().toUtf8().data());
        pinfo.mAvailable = false;
        return false;
    }
    typedef PluginInterface* (*createPlugin) ();
    createPlugin p = (createPlugin)pinfo.mModule->resolve("createSettingsPlugin");
    if (!p) {
        CT_SYSLOG(LOG_ERR, "create module class failed, error: '%s'", pinfo.mModule->errorString().toUtf8().data());
        return false;
    }
    pinfo.mPlugin = (PluginInterface*)p();

    return true;
}
