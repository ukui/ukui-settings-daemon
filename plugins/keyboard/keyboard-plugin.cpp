#include "keyboard-plugin.h"
#include "clib-syslog.h"

PluginInterface * KeyboardPlugin::mInstance=nullptr;
KeyboardManager * KeyboardPlugin::UsdKeyboardManager=nullptr;

KeyboardPlugin::KeyboardPlugin()
{
    syslog_init("ukui-setting-daemon-keyboard",LOG_LOCAL6);
    CT_SYSLOG(LOG_DEBUG,"KeyboardPlugin initializing!");
    if(nullptr == UsdKeyboardManager)
        UsdKeyboardManager = KeyboardManager::KeyboardManagerNew();
}

KeyboardPlugin::~KeyboardPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"Keyboard plugin free");
    if (UsdKeyboardManager){
        delete UsdKeyboardManager;
        UsdKeyboardManager =nullptr;
    }
}

void KeyboardPlugin::activate()
{
    bool res;
    CT_SYSLOG(LOG_DEBUG,"Activating Keyboard Plugin");
    res = UsdKeyboardManager->KeyboardManagerStart();
    if(!res){
        CT_SYSLOG(LOG_ERR,"Unable to start Keyboard Manager!")
    }

}

PluginInterface * KeyboardPlugin::getInstance()
{
    if(nullptr == mInstance){
        mInstance = new KeyboardPlugin();
    }
    return  mInstance;
}

void KeyboardPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating Keyboard Plugin");
    UsdKeyboardManager->KeyboardManagerStop();
}

PluginInterface *creatSettingPlugin()
{
    return KeyboardPlugin::getInstance();
}


