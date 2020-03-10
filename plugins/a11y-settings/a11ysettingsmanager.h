#ifndef A11YSETTINGS_H
#define A11YSETTINGS_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

class  A11ySettingsManager
{
public:
    ~A11ySettingsManager();
    static A11ySettingsManager* A11ySettingsManagerNew();
    bool A11ySettingsManagerStart(GError** error);
    void A11ySettingsMAnagerStop();

    enum {
        PROP_0,
    };

private:
    A11ySettingsManager();
    static void apps_settings_changed(GSettings *settings,
                                      const char *key,
                               A11ySettingsManager* manager);
    static A11ySettingsManager* mA11ySettingsManager;

    GSettings *interface_settings;
    GSettings *a11y_apps_settings;
};

#endif // A11YSETTINGS_H
