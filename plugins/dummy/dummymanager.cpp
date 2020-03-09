#include "dummymanager.h"
#include "clib-syslog.h"
DummyManager* DummyManager::mDummyManager = nullptr;

DummyManager::DummyManager()
{
    //TODO...
    //set get property...
}

DummyManager::~DummyManager()
{
    if(mDummyManager)
        delete mDummyManager;
}

DummyManager& DummyManager::DummyManagerNew()
{
    if(nullptr == mDummyManager)
        mDummyManager = new DummyManager();
    return *mDummyManager;
}

bool DummyManager::DummyManagerStart(GError **error)
{
    CT_SYSLOG(LOG_DEBUG,"Starting Dummy manager!");
    return true;
}

void DummyManager::DummyManagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"Stopping Dummy manager!");
}






