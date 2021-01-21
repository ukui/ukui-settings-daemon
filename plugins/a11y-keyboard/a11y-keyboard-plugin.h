#ifndef A11YKEYBOARDPLUGIN_H
#define A11YKEYBOARDPLUGIN_H

#include "plugin-interface.h"
#include "a11y-keyboard-manager.h"

class A11yKeyboardPlugin :  public PluginInterface
{
public:
    ~A11yKeyboardPlugin();
    static PluginInterface * getInstance();
    virtual void activate();
    virtual void deactivate();

private:
    A11yKeyboardPlugin();
    A11yKeyboardPlugin(A11yKeyboardPlugin&)=delete;

    static A11yKeyboardManager *UsdA11yManager;
    static PluginInterface     *mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // A11YKEYBOARDPLUGIN_H
