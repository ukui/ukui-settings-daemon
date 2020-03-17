#include "a11ysettingsmanager.h"
#include "clib-syslog.h"

bool start_a11y_keyboard_idle(A11ySettingsManager*);

A11ySettingsManager* A11ySettingsManager::mA11ySettingsManager = nullptr;

A11ySettingsManager::A11ySettingsManager()
{
}

A11ySettingsManager* A11ySettingsManager::A11ySettingsManagerNew()
{
    if(nullptr == mA11ySettingsManager)
        mA11ySettingsManager = new A11ySettingsManager();
    return mA11ySettingsManager;
}

bool A11ySettingsManager::A11ySettingsManagerStart(GError**)
{
    //
    CT_SYSLOG(LOG_DEBUG,"Starting a11y_settings manager!");
    interface_settings=g_settings_new("org.mate.interface");
    a11y_apps_settings=g_settings_new("org.gnome.desktop.a11y.applications");

    g_signal_connect(G_OBJECT(a11y_apps_settings),"changed",
                     G_CALLBACK(apps_settings_changed),mA11ySettingsManager);

    /* If any of the screen reader or on-screen keyboard are enabled,
     * make sure a11y is enabled for the toolkits.
     * We don't do the same thing for the reverse so it's possible to
     * enable AT-SPI for the toolkits without using an a11y app */
    if(g_settings_get_boolean(a11y_apps_settings,"screen-keyboard-enabled") ||
       g_settings_get_boolean(a11y_apps_settings,"screen-reader-enabled"))
        g_settings_set_boolean(interface_settings,"accessibility",TRUE);

    return TRUE;
}


void A11ySettingsManager::A11ySettingsMAnagerStop()
{
    if(interface_settings){
        g_object_unref(interface_settings);
        interface_settings = nullptr;
    }
    if(a11y_apps_settings){
        g_object_unref(a11y_apps_settings);
        a11y_apps_settings = nullptr;
    }
    CT_SYSLOG(LOG_DEBUG,"Stoping a11y_settings manager");
}


void A11ySettingsManager::apps_settings_changed(GSettings *settings,
                                  const char *key,
                           A11ySettingsManager* manager){
    gboolean screen_reader,keyboard;
    if(g_str_equal(key,"screen-reader-enabled") == FALSE &&
       g_str_equal(key,"screen-keyboard-enabled") == FALSE)
        return;
    CT_SYSLOG(LOG_DEBUG,"screen reader or OSK enabledment changed");

    screen_reader=g_settings_get_boolean(manager->a11y_apps_settings,"screen-reader-enabled");
    keyboard=g_settings_get_boolean(manager->a11y_apps_settings,"screen-keyboard-enabled");

    if(screen_reader || keyboard){
        CT_SYSLOG(LOG_DEBUG,"Enabling accessibility,screen reader or OSK enabled!");
        g_settings_set_boolean(manager->interface_settings,"accessibility",TRUE);
    }else if(screen_reader == FALSE && keyboard == FALSE){
        CT_SYSLOG(LOG_DEBUG,"Disabling accessibility,screen reader or OSK disabled!");
        g_settings_set_boolean(manager->interface_settings,"accessibility",FALSE);
    }
}

bool start_a11y_keyboard_idle(A11ySettingsManager*)
{

}













