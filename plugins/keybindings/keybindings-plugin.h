#ifndef KEYBINDINGSPLUGIN_H
#define KEYBINDINGSPLUGIN_H

#include "keybindings_global.h"
#include "plugin-interface.h"
#include "keybindings-manager.h"


class KeybindingsPlugin : public PluginInterface
{
public:
    ~KeybindingsPlugin();
    static PluginInterface *getInstance();
    virtual void activate();
    virtual void deactivate();

private:
    KeybindingsPlugin();
    KeybindingsPlugin(KeybindingsPlugin&)=delete;

private:
    static KeybindingsManager *mKeyManager;
    static PluginInterface    *mInstance;

};

extern "C" Q_DECL_EXPORT PluginInterface * createSettingsPlugin();

#endif // KEYBINDINGSPLUGIN_H
