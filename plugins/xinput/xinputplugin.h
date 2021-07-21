#ifndef XINPUTPLUGIN_H
#define XINPUTPLUGIN_H

#include "plugin-interface.h"
#include "xinputmanager.h"

#include <QThread>

extern "C"{
    #include "usd_log.h"
}

class XinputPlugin : public PluginInterface
{
public:
    XinputPlugin();
    XinputPlugin(XinputPlugin&)=delete;
    ~XinputPlugin();
    static XinputPlugin *instance();

    virtual void activate();
    virtual void deactivate();

private:
    XinputManager *m_pXinputManager;
    static XinputPlugin *_instance;
};

extern "C" Q_DECL_EXPORT PluginInterface *createSettingsPlugin();

#endif // XINPUTPLUGIN_H
