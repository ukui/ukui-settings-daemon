#ifndef CLIPBOARDPLUGIN_H
#define CLIPBOARDPLUGIN_H
#include "clipboard-manager.h"
#include "plugin-interface.h"

class ClipboardPlugin : public PluginInterface
{
public:
    ~ClipboardPlugin();
    static PluginInterface* getInstance();

    virtual void activate ();
    virtual void deactivate ();

private:
    ClipboardPlugin();
    ClipboardPlugin(ClipboardPlugin&)=delete;

private:
    static ClipboardManager*        mManager;
    static PluginInterface*         mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // CLIPBOARDPLUGIN_H
