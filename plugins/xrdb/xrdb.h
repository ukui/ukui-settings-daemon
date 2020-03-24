#ifndef XRDB_H
#define XRDB_H

#include "xrdb_global.h"
#include "plugin-interface.h"
#include "ixrdb-manager.h"

class Xrdb : public PluginInterface
{

public:
    Xrdb();
public:
    virtual ~Xrdb() = 0;

    virtual void activate () = 0;
    virtual void deactivate () = 0;

protected:
    QString pluginName;
    IXrdbManager *m_pIXdbMgr;
};

#endif // XRDB_H
