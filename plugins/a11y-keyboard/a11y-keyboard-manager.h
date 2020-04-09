#ifndef A11YKEYBOARDMANAGER_H
#define A11YKEYBOARDMANAGER_H

#include <QObject>
#include <QTimer>
#include <QtX11Extras/QX11Info>
#include "QGSettings/qgsettings.h"
#include <QApplication>

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include <X11/XKBlib.h>
#include <X11/extensions/XKBstr.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif /* HAVE_LIBNOTIFY */

class A11yKeyboardManager : public QObject
{
    Q_OBJECT
private:
    A11yKeyboardManager()=delete ;
    A11yKeyboardManager(A11yKeyboardManager&)=delete ;
    A11yKeyboardManager&operator=(const A11yKeyboardManager&)=delete ;
    A11yKeyboardManager(QObject *parent = nullptr);

public:
    ~A11yKeyboardManager();
    static A11yKeyboardManager *A11KeyboardManagerNew();
    bool   A11yKeyboardManagerStart();
    void   A11yKeyboardManagerStop();

public Q_SLOTS:
    void  start_a11y_keyboard_idle_cb();
    void  keyboard_callback(QString);

public:
    static XkbDescRec  *get_xkb_desc_rec ();
    static void         set_server_from_settings (A11yKeyboardManager *manager);
    static void         maybe_show_status_icon   (A11yKeyboardManager *manager);
    static void         usd_a11y_keyboard_manager_ensure_status_icon(A11yKeyboardManager *manager);
    static void         on_status_icon_activate (GtkStatusIcon  *status_icon,
                                                 A11yKeyboardManager *manager);
    static gboolean     xkb_enabled (A11yKeyboardManager *manager);
    static void         set_devicepresence_handler (A11yKeyboardManager *manager);
    static void         set_settings_from_server   (A11yKeyboardManager *manager);
    static void         restore_server_xkb_config  (A11yKeyboardManager *manager);

private:
    friend GdkFilterReturn cb_xkb_event_filter (GdkXEvent           *xevent,
                                                GdkEvent          *ignored1,
                                                A11yKeyboardManager *manager);
    friend void         ax_slowkeys_warning_post (A11yKeyboardManager *manager,
                                                  gboolean            enabled);
    friend void         ax_stickykeys_warning_post (A11yKeyboardManager *manager,
                                                    gboolean            enabled);

private:
    static A11yKeyboardManager  *mA11yKeyboard;
    QTimer                      *time;
    int                         xkbEventBase;
    gboolean                    stickykeys_shortcut_val;
    gboolean                    slowkeys_shortcut_val;
    GtkWidget                   *stickykeys_alert;
    GtkWidget                   *slowkeys_alert;
    GtkWidget                   *preferences_dialog;
    GtkStatusIcon               *status_icon;
    XkbDescRec                  *original_xkb_desc;
    QGSettings                  *settings;

#ifdef HAVE_LIBNOTIFY
    NotifyNotification *notification;
#endif /* HAVE_LIBNOTIFY */

};

#endif // A11YKEYBOARDMANAGER_H
