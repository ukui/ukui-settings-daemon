#include "typingbreakplugin.h"
#include "clib-syslog.h"

PluginInterface* TypingBreakPlugin::mInstance = nullptr;

TypingBreakPlugin::TypingBreakPlugin()
{
    breakManager=TypingBreakManager::TypingBreakManagerNew();
}

TypingBreakPlugin::~TypingBreakPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"TypingBreakPlugin deconstructor!");
    if(breakManager)
        delete breakManager;
}

void TypingBreakPlugin::activate()
{
    bool res;
    GError* error;
    CT_SYSLOG(LOG_DEBUG,"Activating typing_break plugin!");

    error=NULL;
    res=breakManager->TypingBreakManagerStart(&error);
    if(!res){
        CT_SYSLOG(LOG_DEBUG,"Unable to Start typing_break manager:%s",error->message);
        g_error_free(error);
    }
}

void TypingBreakPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating typing_break plugin!");
    breakManager->TypingBreakManagerStop();
}

PluginInterface* TypingBreakPlugin::getInstance()
{
    if (nullptr == mInstance)
        mInstance = new TypingBreakPlugin();
    return mInstance;
}

PluginInterface* createSettingsPlugin(){
    return TypingBreakPlugin::getInstance();
}



















