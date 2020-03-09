#include "backgroundplugin.h"
#include "clib_syslog.h"

UkuiSettingsPlugin* BackgroundPlugin::mInstance = nullptr;

BackgroundPlugin::BackgroundPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "background plugin init...");
}

BackgroundPlugin::~BackgroundPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "background plugin free...");
}

UkuiSettingsPlugin *BackgroundPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new BackgroundPlugin();
    }
    return mInstance;
}

void BackgroundPlugin::activate()
{
    CT_SYSLOG (LOG_DEBUG, "Activating background plugin");
}

void BackgroundPlugin::deactivate()
{
    CT_SYSLOG (LOG_DEBUG, "Deactivating background plugin");
}

UkuiSettingsPlugin *createSettingsPlugin()
{
    return BackgroundPlugin::getInstance();
}
