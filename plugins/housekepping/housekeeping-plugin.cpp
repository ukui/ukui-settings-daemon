#include "housekeeping-plugin.h"
#include "clib-syslog.h"

PluginInterface     *HousekeepingPlugin::mInstance=nullptr;
HousekeepingManager *HousekeepingPlugin::mHouseManager=nullptr;

HousekeepingPlugin::HousekeepingPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"HousekeppingPlugin initializing!");
    if(nullptr == mHouseManager)
        mHouseManager = HousekeepingManager::HousekeepingManagerNew();
}

HousekeepingPlugin::~HousekeepingPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"HousekeepingPlugin free");
    if(mHouseManager){
        delete mHouseManager;
        mHouseManager = nullptr;
    }
}

void HousekeepingPlugin::activate()
{
    bool res;
    CT_SYSLOG(LOG_DEBUG,"Activating HousekeepingPlugin");
    res = mHouseManager->HousekeepingManagerNew();
    if(!res)
        CT_SYSLOG(LOG_ERR,"Unable to start Housekeeping Manager!");
}

PluginInterface *HousekeepingPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new HousekeepingPlugin();
    return mInstance;
}

void HousekeepingPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating Housekeeping Plugin");
    mHouseManager->HousekeepingManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return HousekeepingPlugin::getInstance();
}
