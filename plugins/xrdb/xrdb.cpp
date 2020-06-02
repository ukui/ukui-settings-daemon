#include "xrdb.h"
#include "ukui-xrdb-manager.h"
#include "clib-syslog.h"

XrdbPlugin* XrdbPlugin::mXrdbPlugin = nullptr;

XrdbPlugin::XrdbPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"XrdbPlugin initializing!");
    m_pIXdbMgr = ukuiXrdbManager::ukuiXrdbManagerNew();
}

XrdbPlugin::~XrdbPlugin() {
    CT_SYSLOG(LOG_DEBUG,"XrdbPlugin deconstructor!");
    if (m_pIXdbMgr) {
        delete m_pIXdbMgr;
    }
    m_pIXdbMgr = nullptr;
}

void XrdbPlugin::activate () {
    GError* error;
    CT_SYSLOG(LOG_DEBUG,"Activating xrdn plugin!");

    error = NULL;
    if(!m_pIXdbMgr->start(&error)){
        CT_SYSLOG(LOG_DEBUG,"unable to start xrdb manager: %s",error->message);
        g_error_free(error);
    }
}

void XrdbPlugin::deactivate () {
    CT_SYSLOG(LOG_DEBUG,"Deactivating xrdn plugin!");
    m_pIXdbMgr->stop();
}

PluginInterface* XrdbPlugin::getInstance()
{
    if(nullptr == mXrdbPlugin)
        mXrdbPlugin = new XrdbPlugin();
    return mXrdbPlugin;
}

PluginInterface* createSettingsPlugin()
{
    return XrdbPlugin::getInstance();
}

