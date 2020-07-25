#include <QDebug>
#include "ukui-xsettings-plugin.h"
#include <syslog.h>
PluginInterface *XSettingsPlugin::mInstance =nullptr;
ukuiXSettingsManager *XSettingsPlugin::m_pukuiXsettingManager = nullptr;

XSettingsPlugin::XSettingsPlugin()
{
    if(nullptr == m_pukuiXsettingManager)
        m_pukuiXsettingManager = new ukuiXSettingsManager();
}

XSettingsPlugin::~XSettingsPlugin()
{
    if (m_pukuiXsettingManager)
        delete m_pukuiXsettingManager;
    m_pukuiXsettingManager = nullptr;
}

void XSettingsPlugin::activate()
{
    bool res;
    res = m_pukuiXsettingManager->start();
    if (!res) {
        qWarning ("Unable to start XSettingsPlugin manager");
    }
}

void XSettingsPlugin::deactivate()
{
    m_pukuiXsettingManager->stop();
}
PluginInterface *XSettingsPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new XSettingsPlugin();
    return mInstance;
}
PluginInterface* createSettingsPlugin() {
    return XSettingsPlugin::getInstance();
}
