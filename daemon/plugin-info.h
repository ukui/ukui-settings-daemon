#ifndef PluginInfo_H
#define PluginInfo_H
#include "plugin-interface.h"

#include <glib-object.h>
#include <QLibrary>
#include <QObject>
#include <string>

#include <QGSettings/qgsettings.h>

namespace UkuiSettingsDaemon {
class PluginInfo;
}

class PluginInfo : public QObject
{
    Q_OBJECT
public:
    explicit PluginInfo()=delete;
    PluginInfo(QString& fileName);
    ~PluginInfo();

    bool pluginActivate ();
    bool pluginDeactivate ();
    bool pluginIsactivate ();
    bool pluginEnabled ();
    bool pluginIsAvailable ();

    QString& getPluginName ();
    QString& getPluginDescription ();
    QList<QString>& getPluginAuthors ();
    QString& getPluginWebsite ();
    QString& getPluginCopyright ();
    QString& getPluginLocation ();

    int& getPluginPriority ();

    void setPluginPriority (int priority);
    void setPluginSchema (QString& schema);

    bool operator== (PluginInfo&);

public Q_SLOTS:
    void pluginSchemaSlot (QString key);

private:
    bool activatePlugin();
    bool loadPluginModule();
    void deactivatePlugin();

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
    QGSettings*             mSettings;

    PluginInterface*        mPlugin;
    QLibrary*               mModule;

    QList<QString>*         mAuthors;
//    char**                  mAuthors;                   // FIXME://
//    guint                   mEnabledNotificationId;
};

#endif // PluginInfo_H
