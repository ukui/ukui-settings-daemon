#ifndef UKUISETTINGSPLUGININFO_H
#define UKUISETTINGSPLUGININFO_H
#include "ukuisettingsplugin.h"

#include <glib-object.h>
#include <gmodule.h>

#include <gio/gio.h>

#include <QLibrary>
#include <QObject>
#include <string>

class UkuiSettingsPluginInfo : public QObject
{
    Q_OBJECT
public:
    UkuiSettingsPluginInfo()=delete;
    UkuiSettingsPluginInfo(QString& fileName); // DD-OKK // ukui_settings_plugin_info_new_from_file (const char *filename);

    bool ukuiSettingsPluginInfoActivate ();
    bool ukuiSettingsPluginInfoDeactivate ();
    bool ukuiSettingsPluginInfoIsactivate ();
    bool ukuiSettingsPluginInfoGetEnabled ();
    bool ukuiSettingsPluginInfoIsAvailable ();

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
    bool activatePlugin();
    bool loadPluginModule();
    void deactivatePlugin();
    bool pluginEnabledCB(GSettings* settings, gchar* key, UkuiSettingsPluginInfo*);

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
    UkuiSettingsPlugin*     mPlugin;
    QLibrary*               mModule;

    char**                  mAuthors;             // FIXME://
    guint                   enabled_notification_id;
};

#endif // UKUISETTINGSPLUGININFO_H
