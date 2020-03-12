#ifndef PluginInfo_H
#define PluginInfo_H
#include "plugin-interface.h"

#include <glib-object.h>
#include <gmodule.h>

#include <gio/gio.h>

#include <QLibrary>
#include <QObject>
#include <string>

namespace UkuiSettingsDaemon {
class PluginInfo;
}

class PluginInfo : public QObject
{
    Q_OBJECT
public:
    explicit PluginInfo()=delete;
    PluginInfo(QString& fileName); // DD-OKK // ukui_settings_plugin_info_new_from_file (const char *filename);

    bool pluginActivate ();
    bool pluginDeactivate ();
    bool pluginIsactivate ();
    bool pluginEnabled ();
    bool pluginIsAvailable ();

    QString& getPluginName ();
    QString& getPluginDescription ();
    const char** getPluginAuthors ();
    QString& getPluginWebsite ();
    QString& getPluginCopyright ();
    QString& getPluginLocation ();

    int& getPluginPriority ();

    void setPluginPriority (int priority);
    void setPluginSchema (QString& schema);

    bool operator== (PluginInfo&);

signals:
    void activated(QString&);
    void deactivated(QString&);

private:
    bool activatePlugin();
    bool loadPluginModule();
    void deactivatePlugin();
//    void pluginEnabledCB(GSettings* settings, gchar* key, PluginInfo*);

private:
    /* Priority determines the order in which plugins are started and stopped. A lower number means higher priority. */
    int                     mPriority;

    bool                    mActive;
    bool                    mEnabled;
    bool                    mAvailable;

    QString                 mFile;
    QString                 mName;
    QString                 mDesc;
    QString                 mWebsite;
    QString                 mLocation;
    QString                 mCopyright;
    GSettings*              mSettings;
    PluginInterface*        mPlugin;
    QLibrary*               mModule;

    char**                  mAuthors;             // FIXME://
    guint                   mEnabledNotificationId;
};

#endif // PluginInfo_H
