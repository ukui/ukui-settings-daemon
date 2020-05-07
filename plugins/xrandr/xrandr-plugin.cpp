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
    CT_SYSLOG(LOG_DEBUG,"activating Xrandr plugins");

    res=UsdXrandrManager->XrandrManagerStart();  //start function
    if(!res){
        CT_SYSLOG(LOG_DEBUG,"Unable to start Xrandr manager!");
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





