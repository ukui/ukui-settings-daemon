#include "backgroundplugin.h"
#include "clib_syslog.h"

BackgroundPlugin::BackgroundPlugin()
{

}

void BackgroundPlugin::activate()
{
    CT_SYSLOG (LOG_DEBUG, "Activating background plugin");
}

void BackgroundPlugin::deactivate()
{
    CT_SYSLOG (LOG_DEBUG, "Deactivating background plugin");
}
