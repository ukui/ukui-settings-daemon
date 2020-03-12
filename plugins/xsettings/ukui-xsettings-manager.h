#ifndef UKUIXSETTINGSMANAGER_H
#define UKUIXSETTINGSMANAGER_H

//#include <QObject>
#include <glib.h>
#include <X11/Xatom.h>

#include <glib/gi18n.h>
#include </usr/include/gtk-3.0/gdk/gdk.h>
#include </usr/include/gtk-3.0/gdk/gdkx.h>
#include </usr/include/gtk-3.0/gtk/gtk.h>
#include <gio/gio.h>
#include "ixsettings-manager.h"
#include "xsettings-manager.h"
#include "fontconfig-monitor.h"


class ukuiXSettingsManager : public IXsettingsManager
{
public:
    ukuiXSettingsManager();
    ~ukuiXSettingsManager();
    int start(GError               **error);
    int stop();

    gboolean setup_xsettings_managers (ukuiXSettingsManager *manager);

    XsettingsManager **pManagers;
    GHashTable *gsettings;
    GSettings *gsettings_font;
    fontconfig_monitor_handle_t *fontconfig_handle;

};

#endif // UKUIXSETTINGSMANAGER_H
