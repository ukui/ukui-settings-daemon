#ifndef SOUNDPLUGIN_H
#define SOUNDPLUGIN_H

#include "soundmanager.h"
#include "plugin-interface.h"

class SoundPlugin : public PluginInterface{
public:
    ~SoundPlugin();
    static PluginInterface* getInstance();

    virtual void activate();
    virtual void deactivate();

private:
    SoundPlugin();
    SoundManager* soundManager;
    static SoundPlugin* mSoundPlugin;
};

extern "C" PluginInterface* Q_DECL_EXPORT createSettingsPlugin();
#endif /*SOUND_PLUGIN_H */
