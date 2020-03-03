#ifndef UKUISETTINGSPLUGIN_H
#define UKUISETTINGSPLUGIN_H

#include <QString>

class UkuiSettingsPlugin
{
public:
    UkuiSettingsPlugin();

    virtual void activate () = 0;
    virtual void deactivate () = 0;

    void ukuiSettingsPluginActivate ();
    void ukuiSettingsPluginDeactivate ();

protected:
    QString pluginName;
};

#endif // UKUISETTINGSPLUGIN_H
