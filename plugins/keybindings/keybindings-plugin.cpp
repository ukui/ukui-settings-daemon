#include "keybindings-plugin.h"
#include "clib-syslog.h"

PluginInterface    *KeybindingsPlugin::mInstance=nullptr;
KeybindingsManager *KeybindingsPlugin::mKeyManager=nullptr;

KeybindingsPlugin::KeybindingsPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"KeybindingsPlugin initalizing");
    if(nullptr == mKeyManager)
        mKeyManager = KeybindingsManager::KeybindingsManagerNew();
}

KeybindingsPlugin::~KeybindingsPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"KeybindingsPlugin free");
    if(mKeyManager){
        delete mKeyManager;
        mKeyManager = nullptr;
    }
}

void KeybindingsPlugin::activate()
{
    bool res;
    CT_SYSLOG(LOG_DEBUG,"Activating Keybindings");
    res = mKeyManager->KeybindingsManagerStart();
    if(!res)
        CT_SYSLOG(LOG_ERR,"Unable to start Keybindings manager");
}

PluginInterface *KeybindingsPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new KeybindingsPlugin();
    return mInstance;
}

void KeybindingsPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Dectivating Keybindings Plugin");
    mKeyManager->KeybindingsManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return KeybindingsPlugin::getInstance();
}
