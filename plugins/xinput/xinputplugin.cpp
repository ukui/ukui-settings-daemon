#include "xinputplugin.h"

#include <QMutex>
#include <syslog.h>

XinputPlugin* XinputPlugin::_instance = nullptr;

XinputPlugin* XinputPlugin::instance()
{
    QMutex mutex;
    mutex.lock();
    if(nullptr == _instance)
        _instance = new XinputPlugin;
    mutex.unlock();
    return _instance;
}

XinputPlugin::XinputPlugin():
    m_pXinputManager(new XinputManager)
{
    USD_LOG(LOG_ERR, "Loading Xinput plugins");
}

XinputPlugin::~XinputPlugin()
{

}

void XinputPlugin::activate()
{
    USD_LOG(LOG_ERR,"activating Xinput plugins");
    m_pXinputManager->start();
}

void XinputPlugin::deactivate()
{
    m_pXinputManager->stop();
}

PluginInterface *createSettingsPlugin()
{
    return dynamic_cast<PluginInterface*>(XinputPlugin::instance());
}

