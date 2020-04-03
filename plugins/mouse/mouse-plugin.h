#ifndef MOUSEPLUGIN_H
#define MOUSEPLUGIN_H

#include "mouse-manager.h"
#include "plugin-interface.h"
#include <QtCore/QtGlobal>

class MousePlugin : public PluginInterface
{
public:
    ~MousePlugin();
    static PluginInterface * getInstance();
    virtual void activate();
    virtual void deactivate();

private:
    MousePlugin();
    MousePlugin(MousePlugin&)=delete;

    static MouseManager *  UsdMouseManager;
    static PluginInterface * mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // MOUSEPLUGIN_H
