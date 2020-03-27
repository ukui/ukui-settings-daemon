#include "smartcard-plugin.h"
#include "usd-smartcard.h"

#include "clib-syslog.h"
#include <gmodule.h>
#include <glib.h>
#include <gio/gio.h>
#include <cstdlib>
typedef enum
{
    USD_SMARTCARD_REMOVE_ACTION_NONE,
    USD_SMARTCARD_REMOVE_ACTION_LOCK_SCREEN,
    USD_SMARTCARD_REMOVE_ACTION_FORCE_LOGOUT,
} UsdSmartcardRemoveAction;

#define USD_SMARTCARD_SCHEMA "org.ukui.peripherals-smartcard"
#define KEY_REMOVE_ACTION "removal-action"

SmartcardManager * SmartcardPlugin::mManager=nullptr;
PluginInterface * SmartcardPlugin::mInstance=nullptr;
Smartcard *       SmartcardPlugin::mSmartcard=nullptr;

SmartcardPlugin::SmartcardPlugin()
{
    if (nullptr == mManager)
        mManager=SmartcardManager::managerNew();
}

SmartcardPlugin::~SmartcardPlugin()
{
    delete mManager;
    mManager = nullptr;
}
PluginInterface * SmartcardPlugin::getInstance()
{
    if(nullptr == mInstance){
        mInstance = new SmartcardPlugin();
    }
    return mInstance;
}
static gboolean
user_logged_in_with_smartcard()
{
    return std::getenv ("PKCS11_LOGIN_TOKEN_NAME") !=NULL;
}

UsdSmartcardRemoveAction
get_configured_remove_action (SmartcardPlugin *plugin) //配置删除操作
{
        GSettings *settings;
        char *remove_action_string;
        UsdSmartcardRemoveAction remove_action;

        settings = g_settings_new (USD_SMARTCARD_SCHEMA);
        remove_action_string = g_settings_get_string (settings,
                                                      KEY_REMOVE_ACTION);

        if (remove_action_string == NULL) {
                g_warning ("UsdSmartcardPlugin unable to get smartcard remove action");
                remove_action = USD_SMARTCARD_REMOVE_ACTION_NONE;
        } else if (strcmp (remove_action_string, "none") == 0) {
                remove_action = USD_SMARTCARD_REMOVE_ACTION_NONE;
        } else if (strcmp (remove_action_string, "lock_screen") == 0) {
                remove_action = USD_SMARTCARD_REMOVE_ACTION_LOCK_SCREEN;
        } else if (strcmp (remove_action_string, "force_logout") == 0) {
                remove_action = USD_SMARTCARD_REMOVE_ACTION_FORCE_LOGOUT;
        } else {
                g_warning ("UsdSmartcardPlugin unknown smartcard remove action");
                remove_action = USD_SMARTCARD_REMOVE_ACTION_NONE;
        }

        g_object_unref (settings);

        return remove_action;
}
void lock_screen(SmartcardPlugin *plugin)
{
    QDBusConnection::systemBus().connect(SCREENSAVER_DBUS_NAME,
                                         SCREENSAVER_DBUS_PATH,
                                         SCREENSAVER_DBUS_INTERFACE,
                                         "Lock",
                                         plugin,NULL);// NULL keep
}
void force_logout(SmartcardPlugin *plugin)
{
    QDBusConnection::systemBus().connect(SCREENSAVER_DBUS_NAME,
                                         SCREENSAVER_DBUS_PATH,
                                         SCREENSAVER_DBUS_INTERFACE,
                                         "Logout",
                                         plugin,NULL);// NULL keep
}
void simulate_user_activity(SmartcardPlugin* plugin)
{
    QDBusConnection::systemBus().connect(SCREENSAVER_DBUS_NAME,
                                         SCREENSAVER_DBUS_PATH,
                                         SCREENSAVER_DBUS_INTERFACE,
                                         "SimulateUserActivity",
                                         plugin,NULL);// NULL keep
}
void process_smartcard_removal (SmartcardPlugin *plugin)//除过程智能卡
{
        UsdSmartcardRemoveAction remove_action;

        g_debug ("UsdSmartcardPlugin processing smartcard removal");
        remove_action = get_configured_remove_action (plugin);

        switch (remove_action)
        {
            case USD_SMARTCARD_REMOVE_ACTION_NONE:
                return;
            case USD_SMARTCARD_REMOVE_ACTION_LOCK_SCREEN:
                lock_screen (plugin);
                break;
            case USD_SMARTCARD_REMOVE_ACTION_FORCE_LOGOUT:
                force_logout (plugin);
                break;
        }
}

void smartcard_removed_cb (SmartcardManager *card_monitor,
                          Smartcard        *card,
                          SmartcardPlugin  *plugin)
{

        char *name;

        name = card->usd_smartcard_get_name (card);
        g_debug ("UsdSmartcardPlugin smart card '%s' removed", name);
        g_free (name);

        if (!card->usd_smartcard_is_login_card (card)) {
                g_debug ("UsdSmartcardPlugin removed smart card was not used to login");
                return;
        }

        process_smartcard_removal (plugin);
}
void smartcard_inserted_cb (SmartcardManager *card_monitor,
                            Smartcard        *card,
                            SmartcardPlugin  *plugin)
{
    char *name;
    name = card->usd_smartcard_get_name (card);
    CT_SYSLOG(LOG_DEBUG,"UsdSmartcardPlugin smart card '%s' inserted", name);
    g_free (name);

    simulate_user_activity (plugin);
}
bool RegisterPluginDbus(SmartcardPlugin& m)
{
    QString PluginDbusName = SM_DBUS_NAME;

}
void SmartcardPlugin::activate()
{

    if(this->is_active){
        CT_SYSLOG(LOG_ERR,"SmartcardPlugin Not activating smartcard plugin, because it's already active");
        return;
    }
    if(!user_logged_in_with_smartcard()){
        CT_SYSLOG(LOG_ERR,"SmartcardPlugin Not activating smartcard plugin, \
                            because user didn't use smartcard to log in")
    	this->is_active = FALSE;
        return;
    }
    CT_SYSLOG(LOG_DEBUG,"dSmartcardPlugin Activating smartcard plugin");
    if(nullptr != mManager) mManager->managerStart();
    g_signal_connect(this->mManager,"motion-acceleration",
                     G_CALLBACK(smartcard_removed_cb),
                     this);
    g_signal_connect (this->mManager,
                              "smartcard-inserted",
                              G_CALLBACK (smartcard_inserted_cb), this);

    if (!mManager->usd_smartcard_manager_login_card_is_inserted (this->mManager)) {
            g_debug ("UsdSmartcardPlugin processing smartcard removal immediately user logged in with smartcard "
                     "and it's not inserted");
            process_smartcard_removal (this);
    }
    this->is_active = TRUE;

}

void SmartcardPlugin::deactivate()
{
    if(!this->is_active){
        CT_SYSLOG(LOG_ERR,"UsdSmartcardPlugin Not deactivating smartcard plugin,\
                            because it's already inactive");
        return;
    }
    CT_SYSLOG(LOG_DEBUG,"SmartcardPlugin Deactivating smartcard plugin");
    if(nullptr != mManager) mManager->managerStop();
    g_signal_handlers_disconnect_by_func ((void *)mManager,
                                          (void *)smartcard_removed_cb, this);

    g_signal_handlers_disconnect_by_func ((void *)mManager,
                                          (void *)smartcard_inserted_cb,this);

    this->is_active = FALSE;
}

PluginInterface * createSettingsPlugin()
{
    return SmartcardPlugin::getInstance();
}
