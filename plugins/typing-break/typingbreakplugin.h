#ifndef TYPINGBREAKPLUGIN_H
#define TYPINGBREAKPLUGIN_H

#include "plugin-interface.h"
#include "typingbreakmanager.h"

class TypingBreakPlugin :public PluginInterface
{
public:
    TypingBreakPlugin();
    ~TypingBreakPlugin();
    static PluginInterface* getInstance();

    void activate();
    void deactivate();

private:
    TypingBreakManager* breakManager;
    static PluginInterface* mInstance;
};

extern "C" PluginInterface* Q_DECL_EXPORT CreateSettingsPlugin();

#endif // TYPINGBREAKPLUGIN_H
