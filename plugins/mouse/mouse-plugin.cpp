#include "mouse-plugin.h"
#include "clib-syslog.h"

PluginInterface * MousePlugin::mInstance = nullptr;
MouseManager * MousePlugin::UsdMouseManager = nullptr;

MousePlugin::MousePlugin()
{

    CT_SYSLOG(LOG_DEBUG,"MousePlugin initializing!");
    if (nullptr == UsdMouseManager)
        UsdMouseManager = MouseManager::MouseManagerNew();
}

MousePlugin::~MousePlugin()
{
    if (UsdMouseManager){
        delete UsdMouseManager;
        UsdMouseManager = nullptr;
    }
}

void MousePlugin::activate()
{
    bool res;
    CT_SYSLOG(LOG_DEBUG,"Activating Mouse plugin");
    res = UsdMouseManager->MouseManagerStart();
    if(!res){
        CT_SYSLOG(LOG_ERR,"Unable to start Mouse manager!");
    }

}

PluginInterface * MousePlugin::getInstance()
{
    if (nullptr == mInstance){
        mInstance = new MousePlugin();
    }
    return mInstance;
}

void MousePlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating Mouse Plugin");
    UsdMouseManager->MouseManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return MousePlugin::getInstance();
}





