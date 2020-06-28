#ifndef XSETTINGS_H
#define XSETTINGS_H
#include "plugin-interface.h"
#include "ukui-xsettings-manager.h"
#include <QtCore/qglobal.h>

class Xsettings: public PluginInterface
{
public:
    Xsettings();
    ~Xsettings();

    void activate();
    void deactivate();

private:

    Xsettings(Xsettings&) = delete;

    ukuiXSettingsManager* m_pukuiXsettingManager;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // XSETTINGS_H
