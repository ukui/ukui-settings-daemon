#include "housekeeping-plugin.h"
#include "clib-syslog.h"

PluginInterface     *HousekeepingPlugin::mInstance=nullptr;
HousekeepingManager *HousekeepingPlugin::mHouseManager=nullptr;

HousekeepingPlugin::HousekeepingPlugin()
{
    syslog(LOG_DEBUG,"HousekeppingPlugin initializing!");
    if(nullptr == mHouseManager)
        mHouseManager = HousekeepingManager::HousekeepingManagerNew();
}

HousekeepingPlugin::~HousekeepingPlugin()
{
    syslog(LOG_DEBUG,"HousekeepingPlugin free");
    if(mHouseManager){
        delete mHouseManager;
        mHouseManager = nullptr;
    }
}

void HousekeepingPlugin::activate()
{
    bool res;
    syslog(LOG_ERR,"liutong Activating HousekeepingPlugin");
    res = mHouseManager->HousekeepingManagerNew();
    if(!res)
        syslog(LOG_ERR,"Unable to start Housekeeping Manager!");
}

PluginInterface *HousekeepingPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new HousekeepingPlugin();
    return mInstance;
}

void HousekeepingPlugin::deactivate()
{
    syslog(LOG_DEBUG,"Deactivating Housekeeping Plugin");
    mHouseManager->HousekeepingManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return HousekeepingPlugin::getInstance();
}
