#ifndef BACKGROUNDPLUGIN_H
#define BACKGROUNDPLUGIN_H
#include <ukuisettingsplugin.h>
//#include <QtCore/QtGlobal>

// Q_DECL_EXPORT

class Q_DECL_EXPORT BackgroundPlugin : public UkuiSettingsPlugin
{
public:
    BackgroundPlugin();
    ~BackgroundPlugin();

    virtual void activate ();
    virtual void deactivate ();

    void registerSettingsPlugin ();
};

#endif // BACKGROUNDPLUGIN_H
