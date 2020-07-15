#include "mediakey-plugin.h"
#include "clib-syslog.h"

PluginInterface* MediakeyPlugin::mInstance = nullptr;

MediakeyPlugin::MediakeyPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "mediakey plugin init...");
    mManager = MediaKeysManager::mediaKeysNew();
}

MediakeyPlugin::~MediakeyPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"MediakeyPlugin deconstructor!");
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
    CT_SYSLOG(LOG_DEBUG, "activating mediakey plugin ...");

    if (!mManager->mediaKeysStart(error)) {
            CT_SYSLOG(LOG_DEBUG,"Unable to start media-keys manager: %s", error->message);
            g_error_free (error);
    }
}

void MediakeyPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG, "deactivating mediakey plugin ...");
    mManager->mediaKeysStop();
}

PluginInterface* createSettingsPlugin()
{
    return MediakeyPlugin::getInstance();
}
