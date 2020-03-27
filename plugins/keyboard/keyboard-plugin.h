#ifndef KEYBOARDPLUGIN_H
#define KEYBOARDPLUGIN_H

#include "keyboard_global.h"
#include "keyboard-manager.h"
#include "plugin-interface.h"


class KeyboardPlugin : public PluginInterface
{
public:
    ~KeyboardPlugin();
    static PluginInterface * getInstance();
    void activate();
    void deactivate();

private:
    KeyboardPlugin();
    KeyboardPlugin(KeyboardPlugin&)=delete;
    static KeyboardManager *UsdKeyboardManager;
    static PluginInterface * mInstance;

};

extern "C" Q_DECL_EXPORT PluginInterface * createSettingsPlugin();

#endif // KEYBOARDPLUGIN_H
