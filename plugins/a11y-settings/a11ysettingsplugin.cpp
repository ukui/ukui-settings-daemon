#include "a11ysettingsplugin.h"
#include "clib-syslog.h"

PluginInterface* A11ySettingsPlugin::mInstance = nullptr;

A11ySettingsPlugin::A11ySettingsPlugin()
{
    syslog_init("ukui-settings-daemon-a11y-settings", LOG_LOCAL6);
    CT_SYSLOG(LOG_DEBUG,"A11SettingsPlugin initializing!");
    settingsManager=A11ySettingsManager::A11ySettingsManagerNew();
}

A11ySettingsPlugin::~A11ySettingsPlugin()
{
    if (settingsManager)
        delete settingsManager;
}

void A11ySettingsPlugin::activate()
{
    bool res;
    GError *error;
    CT_SYSLOG(LOG_DEBUG,"Activating a11y-settings plugin");

    error=NULL;
    res=settingsManager->A11ySettingsManagerStart(&error);
    if(!res){
        CT_SYSLOG(LOG_WARNING,"Unable to start a11y-settings manager!");
        g_error_free(error);
    }
}

PluginInterface *A11ySettingsPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new A11ySettingsPlugin();
    }
    return mInstance;
}

void A11ySettingsPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating a11y-settings plugin!");
    settingsManager->A11ySettingsMAnagerStop();
}

PluginInterface* createSettingsPlugin()
{
    return A11ySettingsPlugin::getInstance();
}
