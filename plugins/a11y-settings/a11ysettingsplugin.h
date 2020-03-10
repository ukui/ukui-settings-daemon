#ifndef A11YSETTINGSPLUGIN_H
#define A11YSETTINGSPLUGIN_H
#include "plugin-interface.h"
#include "a11ysettingsmanager.h"
#include <QtCore/QtGlobal>

class A11ySettingsPlugin : public PluginInterface
{
public:
    ~A11ySettingsPlugin();
    static PluginInterface* getInstance();

    void activate();
    void deactivate();

private:
    A11ySettingsPlugin();
    A11ySettingsPlugin(A11ySettingsPlugin&) = delete;

    A11ySettingsManager*        settingsManager;
    static PluginInterface*     mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // A11YSETTINGSPLUGIN_H
