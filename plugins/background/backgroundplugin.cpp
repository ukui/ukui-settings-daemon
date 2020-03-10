#include "backgroundplugin.h"
#include "clib-syslog.h"

PluginInterface* BackgroundPlugin::mInstance = nullptr;

BackgroundPlugin::BackgroundPlugin()
{
    syslog_init("ukui-settings-daemon-background", LOG_LOCAL6);
    CT_SYSLOG(LOG_DEBUG, "background plugin init...");
}

BackgroundPlugin::~BackgroundPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "background plugin free...");
}

PluginInterface *BackgroundPlugin::getInstance()
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

PluginInterface* createSettingsPlugin()
{
    return BackgroundPlugin::getInstance();
}
