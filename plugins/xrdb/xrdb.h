#ifndef XRDB_H
#define XRDB_H

//#include "xrdb_global.h"
#include "plugin-interface.h"
#include "ixrdb-manager.h"

class XrdbPlugin : public PluginInterface
{

public:
    ~XrdbPlugin();
    static PluginInterface* getInstance();

    virtual void activate ();
    virtual void deactivate ();

protected:
    static XrdbPlugin* mXrdbPlugin;
    IXrdbManager *m_pIXdbMgr;

private:
    XrdbPlugin();
};

extern "C" PluginInterface* Q_DECL_EXPORT createSettingsPlugin();
#endif // XRDB_H
