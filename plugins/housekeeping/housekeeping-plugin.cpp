#include "housekeeping-plugin.h"
#include "clib-syslog.h"

PluginInterface     *HousekeepingPlugin::mInstance=nullptr;

HousekeepingPlugin::HousekeepingPlugin()
{
    mHouseManager = new HousekeepingManager();
    if(!mHouseManager)
        syslog(LOG_ERR,"Unable to start Housekeeping Manager!");
}

HousekeepingPlugin::~HousekeepingPlugin()
{
    if(mHouseManager){
        delete mHouseManager;
        mHouseManager = nullptr;
    }
}

void HousekeepingPlugin::activate()
{
    mHouseManager->HousekeepingManagerStart();

}

PluginInterface *HousekeepingPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new HousekeepingPlugin();
    return mInstance;
}

void HousekeepingPlugin::deactivate()
{
    syslog(LOG_ERR,"Deactivating Housekeeping Plugin");
    mHouseManager->HousekeepingManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return HousekeepingPlugin::getInstance();
}
