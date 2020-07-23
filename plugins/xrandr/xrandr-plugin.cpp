#include "xrandr-plugin.h"
#include <syslog.h>

PluginInterface *XrandrPlugin::mInstance      = nullptr;
XrandrManager   *XrandrPlugin::mXrandrManager = nullptr;

XrandrPlugin::XrandrPlugin()
{
    syslog(LOG_ERR,"Xrandr Plugin initializing");
    if(nullptr == mXrandrManager)
        mXrandrManager = XrandrManager::XrandrManagerNew();
}

XrandrPlugin::~XrandrPlugin()
{
    if(mXrandrManager)
        delete mXrandrManager;
    if(mInstance)
        delete mInstance;
}

void XrandrPlugin::activate()
{
    syslog(LOG_ERR,"activating Xrandr plugins");
    bool res;
    res = mXrandrManager->XrandrManagerStart();
    if(!res)
        syslog(LOG_ERR,"Unable to start Xrandr manager!");

}

PluginInterface *XrandrPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new XrandrPlugin();

    return mInstance;
}

void XrandrPlugin::deactivate()
{
    syslog(LOG_ERR,"Deactivating Xrandr plugin");
    mXrandrManager->XrandrManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return XrandrPlugin::getInstance();
}
