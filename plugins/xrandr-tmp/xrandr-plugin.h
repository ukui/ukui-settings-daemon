#ifndef XRANDRPLUGIN_H
#define XRANDRPLUGIN_H

//#include "xrandr_global.h"
#include "xrandr-manager.h"
#include "plugin-interface.h"
#include <QtCore/QtGlobal>

class XrandrPlugin : public PluginInterface
{
public:
    ~XrandrPlugin();
    static PluginInterface * getInstance();

    void activate();
    void deactivate();

private:
    XrandrPlugin();
    XrandrPlugin(XrandrPlugin&) = delete;

    XrandrManager *             UsdXrandrManager;
    static PluginInterface *    mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // XRANDRPLUGIN_H
