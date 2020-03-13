#include "mprisplugin.h"
#include "clib-syslog.h"

PluginInterface* MprisPlugin::mInstance = nullptr;

MprisPlugin::MprisPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"UsdMprisPlugin initializing!");
    mprisManager = MprisManager::MprisManagerNew();
}

MprisPlugin::~MprisPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"UsdMprisPlugin deconstructor!");
    if(mprisManager)
        delete mprisManager;
}

void MprisPlugin::activate()
{
        gboolean res;
        GError  *error;
        CT_SYSLOG(LOG_DEBUG,"Activating mpris plugin");

        error = NULL;
        res = mprisManager->MprisManagerStart(&error);
        if (! res) {
                CT_SYSLOG(LOG_WARNING,"Unable to start mpris manager: %s", error->message);
                g_error_free (error);
        }
}

void MprisPlugin::deactivate()
{
        CT_SYSLOG(LOG_DEBUG,"Deactivating mpris plugin");
        mprisManager->MprisManagerStop();
}

PluginInterface* MprisPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new MprisPlugin();
    return mInstance;
}

PluginInterface* createSettingsPlugin()
{
    return  MprisPlugin::getInstance();
}
