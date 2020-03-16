#ifndef BACKGROUNDPLUGIN_H
#define BACKGROUNDPLUGIN_H
#include "background-manager.h"
#include "plugin-interface.h"
#include <QtCore/QtGlobal>

class BackgroundPlugin : public PluginInterface
{
public:
    ~BackgroundPlugin();
    static PluginInterface* getInstance();

    virtual void activate ();
    virtual void deactivate ();

private:
    BackgroundPlugin();
    BackgroundPlugin(BackgroundPlugin&)=delete;

private:
    static BackgroundManager*       mManager;
    static PluginInterface*         mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // BACKGROUNDPLUGIN_H
