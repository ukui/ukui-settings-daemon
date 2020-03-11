#ifndef TYPINGBREAKPLUGIN_H
#define TYPINGBREAKPLUGIN_H
#include "plugin-interface.h"
#include "typingbreakmanager.h"
#include <QtCore/qglobal.h>

class TypingBreakPlugin :public PluginInterface
{
public:
    ~TypingBreakPlugin();
    static PluginInterface* getInstance();

    virtual void activate();
    virtual void deactivate();

private:
    TypingBreakPlugin();
    TypingBreakPlugin(TypingBreakPlugin&) = delete;

    TypingBreakManager*         breakManager;
    static PluginInterface*     mInstance;
};

extern "C" PluginInterface* Q_DECL_EXPORT createSettingsPlugin();

#endif // TYPINGBREAKPLUGIN_H
