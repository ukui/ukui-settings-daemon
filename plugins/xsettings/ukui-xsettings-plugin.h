#ifndef XSETTINGS_H
#define XSETTINGS_H
#include "plugin-interface.h"
#include "ukui-xsettings-manager.h"
#include <QtCore/qglobal.h>

class XSettingsPlugin: public PluginInterface
{
private:
    XSettingsPlugin();
    XSettingsPlugin(XSettingsPlugin&) = delete;

public: 
    ~XSettingsPlugin();
    static PluginInterface *getInstance();
    virtual void activate();
    virtual void deactivate();

private:
    static ukuiXSettingsManager *m_pukuiXsettingManager;
    static PluginInterface      *mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // XSETTINGS_H
