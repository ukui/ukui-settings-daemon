#ifndef HOUSEKEPPINGPLUGIN_H
#define HOUSEKEPPINGPLUGIN_H

#include "housekeeping_global.h"
#include "housekeeping-manager.h"
#include "plugin-interface.h"

class HousekeepingPlugin : public PluginInterface
{
public:
    ~HousekeepingPlugin();
    static PluginInterface *getInstance();
    virtual void activate();
    virtual void deactivate();

private:
    HousekeepingPlugin();
    HousekeepingPlugin(HousekeepingPlugin&)=delete;

private:
    static HousekeepingManager *mHouseManager;
    static PluginInterface     *mInstance;

};

extern "C" Q_DECL_EXPORT PluginInterface *createSettingsPlugin();

#endif // HOUSEKEPPINGPLUGIN_H
