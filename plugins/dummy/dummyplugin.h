#ifndef DUMMY_H
#define DUMMY_H

//#include "dummy_global.h"
#include "plugin-interface.h"
#include "dummymanager.h"
#include "clib-syslog.h"

class DummyPlugin : public PluginInterface
{
public:
    DummyPlugin();
    ~DummyPlugin();

    virtual void activate();
    virtual void deactivate();

    DummyManager* dummyManager;

private:
    DummyPlugin(DummyPlugin&)=delete;


};

Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // DUMMY_H
