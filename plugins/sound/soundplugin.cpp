#include "soundplugin.h"
#include "clib-syslog.h"

SoundPlugin* SoundPlugin::mSoundPlugin = nullptr;
SoundPlugin::SoundPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"UsdSoundPlugin initializing!");
    soundManager = SoundManager::SoundManagerNew();
}

SoundPlugin::~SoundPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"UsdSoundPlugin deconstructor!");
    if(soundManager)
        delete soundManager;
}

void SoundPlugin::activate ()
{
        GError *error = NULL;
        CT_SYSLOG(LOG_DEBUG,"Activating sound plugin!");

        if (!soundManager->SoundManagerStart(&error)) {
                CT_SYSLOG(LOG_DEBUG,"Unable to start sound manager: %s", error->message);
                g_error_free (error);
        }
}

void SoundPlugin::deactivate ()
{
        CT_SYSLOG(LOG_DEBUG,"Deactivating sound plugin!");
        soundManager->SoundManagerStop();
}

PluginInterface* SoundPlugin::getInstance()
{
    if(nullptr == mSoundPlugin)
        mSoundPlugin = new SoundPlugin();
    return mSoundPlugin;
}

PluginInterface* createSettingsPlugin()
{
    return SoundPlugin::getInstance();
}

