#include "xrandr-plugin.h"
#include "clib-syslog.h"

PluginInterface * XrandrPlugin::mInstance = nullptr;

XrandrPlugin::XrandrPlugin()
{
    syslog_init("ukui-settings-daemon-xrandr", LOG_LOCAL6);
    CT_SYSLOG(LOG_DEBUG,"A11SettingsPlugin initializing!");
    UsdXrandrManager=XrandrManager::XrandrManagerNew(); //new function
}

XrandrPlugin::~XrandrPlugin()
{
    if ( UsdXrandrManager )
        delete UsdXrandrManager ;
}

void XrandrPlugin::activate()
{
    bool res;
    GError *error;
    CT_SYSLOG(LOG_DEBUG,"activating Xrandr plugins");
    error = NULL;
    res=UsdXrandrManager->XrandrManagerStart(&error);  //start function
    if(!res){
        CT_SYSLOG(LOG_DEBUG,"Unable to start Xrandr manager!");
        g_error_free(error);
    }
}

PluginInterface *XrandrPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new XrandrPlugin();
    }
    return mInstance;
}

void XrandrPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating Xrandr plugin");
    UsdXrandrManager->XrandrManagerStop();    //stop function
}

PluginInterface *createSettingsPlugin()
{
    return XrandrPlugin::getInstance();
}





