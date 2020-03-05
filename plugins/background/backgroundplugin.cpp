#include "backgroundplugin.h"
#include "clib_syslog.h"

UkuiSettingsPlugin* BackgroundPlugin::mInstance = nullptr;

BackgroundPlugin::BackgroundPlugin()
{

}

BackgroundPlugin::~BackgroundPlugin()
{

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
