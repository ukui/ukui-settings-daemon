#ifndef XSETTINGSMANAGER_H
#define XSETTINGSMANAGER_H
#include "ixsettings-manager.h"
#include <X11/Xlib.h>
#include "xsettings-common.h"

typedef void (*XSettingsTerminateFunc)  (void *cb_data);

class XsettingsManager : public IXsettingsManager
{
public:
    XsettingsManager();
    ~XsettingsManager();
    int start();
    int stop();

    Window get_window    ();
    Bool   process_event (XEvent           *xev);
    XSettingsResult delete_setting (const char       *name);
    XSettingsResult set_setting    (XSettingsSetting *setting);
    XSettingsResult set_int        (const char       *name,
                                    int               value);
    XSettingsResult set_string     (const char       *name,
                                    const char       *value);
    XSettingsResult set_color      (const char       *name,
                                    XSettingsColor   *value);
    void setting_store (XSettingsSetting *setting,
               XSettingsBuffer *buffer);
    XSettingsResult notify         ();

private:
    Display *display;
    int screen;

    Window window;
    Atom manager_atom;
    Atom selection_atom;
    Atom xsettings_atom;

    XSettingsTerminateFunc terminate;
    void *cb_data;

    XSettingsList *settings;
    unsigned long serial;
};

#endif // XSETTINGSMANAGER_H
