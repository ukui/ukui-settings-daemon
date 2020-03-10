#ifndef A11YSETTINGSPLUGIN_H
#define A11YSETTINGSPLUGIN_H

#include "plugin-interface.h"
#include "a11y-settings_global.h"
#include "a11ysettingsmanager.h"

class A11YSETTINGS_EXPORT A11ySettingsPlugin : public PluginInterface
{
public:
    A11ySettingsPlugin();
    ~A11ySettingsPlugin();

    void activate();
    void deactivate();
private:
    A11ySettingsManager* settingsManager;
};

#endif // A11YSETTINGSPLUGIN_H
