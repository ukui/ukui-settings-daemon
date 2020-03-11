#ifndef XSETTINGS_H
#define XSETTINGS_H
#include "plugin-interface.h"
#include "ixsettings-manager.h"
#include "xsettings_global.h"

class Xsettings: public PluginInterface
{
public:
    Xsettings();
    ~Xsettings();

    static PluginInterface* getInstance();

    void activate();
    void deactivate();

protected:
    static IXsettingsManager *pXsettingManager;
};

#endif // XSETTINGS_H
