#ifndef A11YKEYBOARDMANAGER_H
#define A11YKEYBOARDMANAGER_H
#include <QApplication>
#include <QObject>
#include <QTimer>
#include <QWidget>
#include <QMessageBox>
#include <QtX11Extras/QX11Info>
#include "QGSettings/qgsettings.h"

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
#include "config.h"
#include "a11y-preferences-dialog.h"

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
    void  StartA11yKeyboardIdleCb();
    void  KeyboardCallback(QString);
    void  OnPreferencesDialogResponse();
    void  ax_stickykeys_response(QAbstractButton *button);
    void  ax_slowkeys_response (QAbstractButton *button);

public:
    static XkbDescRec  *GetXkbDescRec ();
    static void SetServerFromSettings (A11yKeyboardManager *manager);
    static void MaybeShowStatusIcon   (A11yKeyboardManager *manager);
    static void A11yKeyboardManagerEnsureStatusIcon(A11yKeyboardManager *manager);
    static void OnStatusIconActivate (GtkStatusIcon  *status_icon,
                                      A11yKeyboardManager *manager);
    static bool XkbEnabled (A11yKeyboardManager *manager);
    static void SetDevicepresenceHandler (A11yKeyboardManager *manager);
    static void SetSettingsFromServer   (A11yKeyboardManager *manager);
    static void RestoreServerXkbConfig  (A11yKeyboardManager *manager);
    static bool AxResponseCallback (A11yKeyboardManager *manager,
                                    QMessageBox     *parent,
                                    int             response_id,
                                    unsigned int    revert_controls_mask,
                                    bool            enabled);
    static void AxSlowkeysWarningPost (A11yKeyboardManager *manager, bool enabled);
    static void AxStickykeysWarningPost (A11yKeyboardManager *manager,bool enabled);
    static void AxStickykeysWarningPostDialog (A11yKeyboardManager *manager,
                                               bool             enabled);
    static void AxSlowkeysWarningPostDialog (A11yKeyboardManager *manager,
                                                 bool               enabled);
private:
    friend GdkFilterReturn CbXkbEventFilter (GdkXEvent           *xevent,
                                             GdkEvent          *ignored1,
                                             A11yKeyboardManager *manager);
    friend void OnNotificationClosed (NotifyNotification     *notification,
                                      A11yKeyboardManager    *manager);

    friend bool AxSlowkeysWarningPostDubble (A11yKeyboardManager *manager, bool enabled);


    friend void on_sticky_keys_action (NotifyNotification     *notification,
                                       const char             *action,
                                       A11yKeyboardManager *manager);
    friend void on_slow_keys_action (NotifyNotification     *notification,
                                     const char             *action,
                                     A11yKeyboardManager *manager);


    friend bool AxStickykeysWarningPostBubble (A11yKeyboardManager *manager,
                                                bool                enabled);

private:
    static A11yKeyboardManager  *mA11yKeyboard;
    QTimer                      *time;
    int                         xkbEventBase;
    bool                        StickykeysShortcutVal;
    bool                        SlowkeysShortcutVal;
    QMessageBox                 *StickykeysAlert = nullptr;
    QMessageBox                 *SlowkeysAlert = nullptr;
    A11yPreferencesDialog       *preferences_dialog=nullptr;
    //GtkStatusIcon               *status_icon;
    XkbDescRec                  *OriginalXkbDesc;
    QGSettings                  *settings;

#ifdef HAVE_LIBNOTIFY
    NotifyNotification *notification;
#endif /* HAVE_LIBNOTIFY */

};

#endif // A11YKEYBOARDMANAGER_H
