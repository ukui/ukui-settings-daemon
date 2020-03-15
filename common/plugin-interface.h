#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <QString>

namespace UkuiSettingsDaemon {
class PluginInterface;
}

class PluginInterface
{
public:
    virtual ~PluginInterface() = 0;

    virtual void activate () = 0;
    virtual void deactivate () = 0;
};

#endif // PLUGIN_INTERFACE_H
