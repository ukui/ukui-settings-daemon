#ifndef MPRISPLUGIN_H
#define MPRISPLUGIN_H

#include "mprismanager.h"
#include "plugin-interface.h"


class MprisPlugin : public PluginInterface{
public:
    MprisPlugin();
    static PluginInterface* getInstance();
    virtual void activate();
    virtual void deactivate();
private:
    ~MprisPlugin();

    MprisManager* mprisManager;
    static PluginInterface*  mInstance;
};

extern "C" PluginInterface* Q_DECL_EXPORT createSettingsPlugin();

#endif /* MPRISPLUGIN_H */
