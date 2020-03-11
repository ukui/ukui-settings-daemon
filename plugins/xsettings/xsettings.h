#ifndef XSETTINGS_H
#define XSETTINGS_H
#include "plugin-interface.h"
//#include "ixsettings-manager.h"
#include <QtCore/qglobal.h>

class Xsettings: public PluginInterface
{
public:
    ~Xsettings();
    static PluginInterface* getInstance();

    void activate();
    void deactivate();

private:
    Xsettings();
    Xsettings(Xsettings&) = delete;
//    IXsettingsManager* mXsettingManager;

    static PluginInterface* mXsettings;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // XSETTINGS_H
