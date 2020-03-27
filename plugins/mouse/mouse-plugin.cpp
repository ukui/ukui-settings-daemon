#include "mouse-plugin.h"
#include "clib-syslog.h"

PluginInterface * MousePlugin::mInstance = nullptr;
MouseManager * MousePlugin::UsdMouseManager = nullptr;

MousePlugin::MousePlugin()
{
    syslog_init("ukui-setting-daemon-mouse",LOG_LOCAL6);
    CT_SYSLOG(LOG_DEBUG,"MousePlugin initializing!");
    if (nullptr == UsdMouseManager)
        UsdMouseManager = MouseManager::MouseManagerNew();
}

MousePlugin::~MousePlugin()
{
     CT_SYSLOG(LOG_DEBUG, "mouse plugin free...");
    if (UsdMouseManager){
        delete UsdMouseManager;
        UsdMouseManager = nullptr;
    }
}

void MousePlugin::activate()
{
    bool res;
    GError *error;
    CT_SYSLOG(LOG_DEBUG,"Activating Mouse plugin");
    error=NULL;
    res = UsdMouseManager->MouseManagerStart(&error);
    if(!res){
        CT_SYSLOG(LOG_ERR,"Unable to start Mouse manager!");
        g_error_free(error);
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

PluginInterface *createSettingPluign()
{
    return MousePlugin::getInstance();
}





