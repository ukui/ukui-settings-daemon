#include "mediakey-plugin.h"
#include "clib-syslog.h"

PluginInterface* MediakeyPlugin::mInstance = nullptr;

MediakeyPlugin::MediakeyPlugin()
{
    syslog(LOG_ERR, "mediakey plugin init...");
    mManager = MediaKeysManager::mediaKeysNew();
}

MediakeyPlugin::~MediakeyPlugin()
{
    syslog(LOG_ERR,"MediakeyPlugin deconstructor!");
    if(mManager){
        delete mManager;
        mManager = nullptr;
    }
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
    GError *error = NULL;
    syslog(LOG_ERR, "activating mediakey plugin ...");

    if (!mManager->mediaKeysStart(error)) {
            syslog(LOG_DEBUG,"Unable to start media-keys manager: %s", error->message);
            g_error_free (error);
    }
}

void MediakeyPlugin::deactivate()
{
    syslog(LOG_ERR, "deactivating mediakey plugin ...");
    mManager->mediaKeysStop();
}

PluginInterface* createSettingsPlugin()
{
    return MediakeyPlugin::getInstance();
}
