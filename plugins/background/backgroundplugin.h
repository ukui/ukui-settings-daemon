#ifndef BACKGROUNDPLUGIN_H
#define BACKGROUNDPLUGIN_H
#include <ukuisettingsplugin.h>
//#include <QtCore/QtGlobal>

// Q_DECL_EXPORT

class Q_DECL_EXPORT BackgroundPlugin : public UkuiSettingsPlugin
{
public:
    ~BackgroundPlugin();
    static UkuiSettingsPlugin* getInstance();

private:
    BackgroundPlugin();
    BackgroundPlugin(BackgroundPlugin&)=delete;

    virtual void activate ();
    virtual void deactivate ();

private:
    static UkuiSettingsPlugin* mInstance;
};

// create a plugin
Q_DECL_EXPORT UkuiSettingsPlugin* createSettingsPlugin();


#endif // BACKGROUNDPLUGIN_H
