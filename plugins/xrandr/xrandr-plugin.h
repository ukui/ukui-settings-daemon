#ifndef XRANDRPLUGIN_H
#define XRANDRPLUGIN_H

#include "xrandr_global.h"
#include "plugin-interface.h"
#include "xrandr-manager.h"

class XrandrPlugin : public PluginInterface
{
private:
    XrandrPlugin();
    XrandrPlugin(XrandrPlugin&)=delete;

public:
    ~XrandrPlugin();
    static PluginInterface *getInstance();

    virtual void activate();
    virtual void deactivate();

private:
    static XrandrManager   *mXrandrManager;
    static PluginInterface *mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface *createSettingsPlugin();

#endif // XRANDRPLUGIN_H
