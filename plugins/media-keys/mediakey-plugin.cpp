#include "mediakey-plugin.h"
#include "clib-syslog.h"

PluginInterface* MediakeyPlugin::mInstance = nullptr;

MediakeyPlugin::MediakeyPlugin()
{
    syslog_init("ukui-settings-daemon-mediakey", LOG_LOCAL6);
    CT_SYSLOG(LOG_DEBUG, "mediakey plugin init...");
//    if (nullptr == )
}

PluginInterface *MediakeyPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new MediakeyPlugin();
    }

    return mInstance;
}

void MediakeyPlugin::activate()
{
    CT_SYSLOG(LOG_DEBUG, "activating mediakey plugin ...");
//    if (!)
}

void MediakeyPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG, "deactivating mediakey plugin ...");
}

PluginInterface* createSettingsPlugin()
{
    return MediakeyPlugin::getInstance();
}
