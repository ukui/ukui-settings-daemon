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

    bool pluginEnabled ();
    bool pluginActivate ();
    bool pluginDeactivate ();
    bool pluginIsactivate ();
    bool pluginIsAvailable ();

    int getPluginPriority ();
    QString& getPluginName ();
    QString& getPluginWebsite ();
    QString& getPluginLocation ();
    QString& getPluginCopyright ();
    QString& getPluginDescription ();
    QList<QString>& getPluginAuthors ();

    void setPluginPriority (int priority);
    void setPluginSchema (QString& schema);

    bool operator== (PluginInfo&);

public Q_SLOTS:
    void pluginSchemaSlot (QString key);

private:
    friend bool loadPluginModule(PluginInfo&);

private:
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

    QLibrary*               mModule;
    PluginInterface*        mPlugin;

    QList<QString>*         mAuthors;
};

#endif // PluginInfo_H
