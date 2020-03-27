#ifndef SMARTCARDPLUGIN_H
#define SMARTCARDPLUGIN_H

#include <QDBusConnectionInterface>
#include <QtDBus/QDBusConnection>

#include "smartcard_global.h"
#include "smartcard-manager.h"
#include "plugin-interface.h"

#define SCREENSAVER_DBUS_NAME      "org.ukui.ScreenSaver"
#define SCREENSAVER_DBUS_PATH      "/"
#define SCREENSAVER_DBUS_INTERFACE "org.ukui.ScreenSaver"

#define SM_DBUS_NAME      "org.gnome.SessionManager"
#define SM_DBUS_PATH      "/org/gnome/SessionManager"
#define SM_DBUS_INTERFACE "org.gnome.SessionManager"

class SmartcardPlugin : public PluginInterface, public QObject
{
    Q_CLASSINFO("D-BUS Interface",SM_DBUS_NAME)

private:
    SmartcardPlugin();
    SmartcardPlugin(SmartcardPlugin&)=delete;

    friend void lock_screen(SmartcardPlugin *plugin);

public:
    ~SmartcardPlugin();
    static PluginInterface *getInstance();
    virtual void activate();
    virtual void deactivate();

private:
    static SmartcardManager * mManager;
    static Smartcard       *mSmartcard;
    static PluginInterface * mInstance;
    guint32              is_active : 1;

};

extern "C" Q_DECL_EXPORT PluginInterface * createSettingsPlugin();

#endif // SMARTCARDPLUGIN_H
