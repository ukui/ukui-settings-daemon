#ifndef UKUISETTINGSPLUGIN_H
#define UKUISETTINGSPLUGIN_H

#include <QString>

class UkuiSettingsPlugin
{
public:
    UkuiSettingsPlugin();
    virtual ~UkuiSettingsPlugin();

    virtual void activate () = 0;
    virtual void deactivate () = 0;

protected:
    QString pluginName;
};

#endif // UKUISETTINGSPLUGIN_H
