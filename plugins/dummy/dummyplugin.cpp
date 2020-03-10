#include "dummyplugin.h"
#include "clib-syslog.h"
DummyPlugin::DummyPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"DummyPlugin initializing!");
    dummyManager=&(DummyManager::DummyManagerNew());
}

void DummyPlugin::activate()
{
    bool res;
    GError* error;
    CT_SYSLOG(LOG_DEBUG,"Activating dummy plugin!");

    error=NULL;
    res=dummyManager->DummyManagerStart(&error);
    if(!res){
        CT_SYSLOG(LOG_DEBUG,"Unable to start dummy manager");
        g_error_free(error);
    }
}

void DummyPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating dummy plugin!");
    dummyManager->DummyManagerStop();
}
PluginInterface* createSettingsPlugin()
{
    return new DummyPlugin();
}



