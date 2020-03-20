#include "backgroundplugin.h"
#include "clib-syslog.h"

BackgroundManager* BackgroundPlugin::mManager = nullptr;
PluginInterface* BackgroundPlugin::mInstance = nullptr;

BackgroundPlugin::BackgroundPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "background plugin init...");
    if (nullptr == mManager) {
        mManager = BackgroundManager::getInstance();
    }
}

BackgroundPlugin::~BackgroundPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "background plugin free...");
    if (nullptr != mManager) {delete mManager; mManager = nullptr;}
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
    if (!mManager->managerStart()) {
        CT_SYSLOG(LOG_ERR, "Unable to start background manager!");
    }
}

void BackgroundPlugin::deactivate()
{
    CT_SYSLOG (LOG_DEBUG, "Deactivating background plugin");
    if (nullptr != mManager) {
        mManager->managerStop();
    }
}

PluginInterface* createSettingsPlugin()
{
    return BackgroundPlugin::getInstance();
}
