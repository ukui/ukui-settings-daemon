#include "a11y-keyboard-manager.h"
#include "clib-syslog.h"

#define CONFIG_SCHEMA "org.mate.accessibility-keyboard"
#define NOTIFICATION_TIMEOUT 30

static GdkFilterReturn DevicepresenceFilter (GdkXEvent *xevent, GdkEvent  *event,gpointer   data);
GdkFilterReturn CbXkbEventFilter (GdkXEvent  *xevent, GdkEvent *ignored1, A11yKeyboardManager *manager);

A11yKeyboardManager *A11yKeyboardManager::mA11yKeyboard=nullptr;

A11yKeyboardManager::A11yKeyboardManager(QObject *parent) : QObject(parent)
{
    time     = new QTimer(this);
    settings = new QGSettings(CONFIG_SCHEMA);
}
A11yKeyboardManager::~A11yKeyboardManager()
{
    delete settings;
    delete time;
}
A11yKeyboardManager * A11yKeyboardManager::A11KeyboardManagerNew()
{
    if(nullptr == mA11yKeyboard)
        mA11yKeyboard = new A11yKeyboardManager(nullptr);
    return mA11yKeyboard;
}

bool A11yKeyboardManager::A11yKeyboardManagerStart()
{
    qDebug(" A11y Keyboard Manager Start ");

    connect(time,SIGNAL(timeout()),this,SLOT(StartA11yKeyboardIdleCb()));
    time->start();

    return true;
}

void A11yKeyboardManager::A11yKeyboardManagerStop()
{
   CT_SYSLOG(LOG_DEBUG,"Stopping A11y Keyboard manager");
   gdk_window_remove_filter (NULL, DevicepresenceFilter, this);

    /*if (status_icon)
       gtk_status_icon_set_visible (status_icon, false);
    */
    gdk_window_remove_filter (NULL,
                         (GdkFilterFunc) CbXkbEventFilter,
                         this);

    /* Disable all the AccessX bits
    */
    RestoreServerXkbConfig (this);
    if (SlowkeysAlert != NULL)
        delete SlowkeysAlert;

    if (SlowkeysAlert != NULL)
        delete SlowkeysAlert;

    SlowkeysShortcutVal = false;
    StickykeysShortcutVal = false;

}

static int GetInt (QGSettings  *settings,
                    char const *key)
{
        int res = settings->get(key).toInt();
        if (res <= 0) {
                res = 1;
        }
        return res;
}

static bool
SetInt (QGSettings      *settings,
         char const     *key,
         int             val)
{
    int  pre_val = settings->get(key).toInt();
    settings->set(key,val);

#ifdef DEBUG_ACCESSIBILITY
        if (val != pre_val) {
                g_warning ("%s changed", key);
        }
#endif
        return val != pre_val;
}

static bool
SetBool (QGSettings      *settings,
          char const     *key,
          int             val)
{
        bool bval = (val != 0);
        bool pre_val = settings->get(key).toBool();

        settings->set(key,bval ? true : false);

#ifdef DEBUG_ACCESSIBILITY
        if (bval != pre_val) {
                d ("%s changed", key);
                return true;
        }
#endif
        return (bval != pre_val);
}

static unsigned long SetClear(bool flag, unsigned long value, unsigned long mask)
{
     if(flag)
         return value | mask;
     return value & ~mask;
}

void A11yKeyboardManager::RestoreServerXkbConfig (A11yKeyboardManager *manager)
{
        gdk_x11_display_error_trap_push (gdk_display_get_default());
        XkbSetControls (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                        XkbSlowKeysMask         |
                        XkbBounceKeysMask       |
                        XkbStickyKeysMask       |
                        XkbMouseKeysMask        |
                        XkbMouseKeysAccelMask   |
                        XkbAccessXKeysMask      |
                        XkbAccessXTimeoutMask   |
                        XkbAccessXFeedbackMask  |
                        XkbControlsEnabledMask,
                        manager->OriginalXkbDesc);

        XkbFreeKeyboard (manager->OriginalXkbDesc,
                         XkbAllComponentsMask, True);

        XSync (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), false);
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default());

        manager->OriginalXkbDesc = NULL;
}

XkbDescRec * A11yKeyboardManager::GetXkbDescRec()
{
        XkbDescRec *desc = NULL;
        Status      status = Success;

        gdk_x11_display_error_trap_push (gdk_display_get_default());
        desc = XkbGetMap (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), XkbAllMapComponentsMask, XkbUseCoreKbd);
        if (desc != NULL) {
                desc->ctrls = NULL;
                status = XkbGetControls (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), XkbAllControlsMask, desc);
        } else {
            desc = NULL;
            gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default());
            return NULL;
        }
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default());

        g_return_val_if_fail (desc != NULL, NULL);
        g_return_val_if_fail (desc->ctrls != NULL, NULL);
        g_return_val_if_fail (status == Success, NULL);

        return desc;
}
static bool SetCtrlFromSettings (XkbDescRec   *desc,
                                 QGSettings  *settings,
                                 char const   *key,
                                 unsigned long mask)
{
       bool result = settings->get(key).toBool();
       desc->ctrls->enabled_ctrls = SetClear(result, desc->ctrls->enabled_ctrls, mask);
       return result;
}

void A11yKeyboardManager::SetServerFromSettings(A11yKeyboardManager *manager)
{
    XkbDescRec      *desc;
    bool        enable_accessX;
    desc = GetXkbDescRec ();
    if (!desc) {
        return;
    }
    /* general */
    enable_accessX = manager->settings->get("enable").toBool();

    desc->ctrls->enabled_ctrls = SetClear (enable_accessX,
                                           desc->ctrls->enabled_ctrls,
                                           XkbAccessXKeysMask);

    if (SetCtrlFromSettings (desc, manager->settings, "timeout-enable",
                             XkbAccessXTimeoutMask)) {
            desc->ctrls->ax_timeout = GetInt (manager->settings, "timeout");
            /* disable only the master flag via the server we will disable
             * the rest on the rebound without affecting gsettings state
             * don't change the option flags at all.
             */
            desc->ctrls->axt_ctrls_mask = XkbAccessXKeysMask | XkbAccessXFeedbackMask;
            desc->ctrls->axt_ctrls_values = 0;
            desc->ctrls->axt_opts_mask = 0;
    }

    desc->ctrls->ax_options = SetClear (manager->settings->get("feature-state-change-beep").toBool(),
                                         desc->ctrls->ax_options,
                                         XkbAccessXFeedbackMask | XkbAX_FeatureFBMask | XkbAX_SlowWarnFBMask);

    /* bounce keys */
    if (SetCtrlFromSettings (desc,
                             manager->settings,
                             "bouncekeys-enable",
                             XkbBounceKeysMask)) {
            desc->ctrls->debounce_delay  = GetInt (manager->settings,
                                                   "bouncekeys-delay");
            desc->ctrls->ax_options = SetClear (manager->settings->get("bouncekeys-beep-reject").toBool(),
                                                 desc->ctrls->ax_options,
                                                 XkbAccessXFeedbackMask | XkbAX_BKRejectFBMask);
    }
    /* mouse keys */
    if (SetCtrlFromSettings (desc,
                             manager->settings,
                             "mousekeys-enable",
                             XkbMouseKeysMask | XkbMouseKeysAccelMask)) {
            desc->ctrls->mk_interval     = 100;     /* msec between mousekey events */
            desc->ctrls->mk_curve        = 50;

            /* We store pixels / sec, XKB wants pixels / event */
            desc->ctrls->mk_max_speed = GetInt (manager->settings,
                                                "mousekeys-max-speed") /
                                                (1000 / desc->ctrls->mk_interval);
            if (desc->ctrls->mk_max_speed <= 0)
                    desc->ctrls->mk_max_speed = 1;

            desc->ctrls->mk_time_to_max = GetInt (manager->settings,
                                                  /* events before max */
                                                  "mousekeys-accel-time") /
                                                  desc->ctrls->mk_interval;
            if (desc->ctrls->mk_time_to_max <= 0)
                    desc->ctrls->mk_time_to_max = 1;

            desc->ctrls->mk_delay = GetInt (manager->settings,
                                            /* ms before 1st event */
                                            "mousekeys-init-delay");
    }

    /* slow keys */
    if (SetCtrlFromSettings (desc,
                             manager->settings,
                             "slowkeys-enable",
                             XkbSlowKeysMask)) {
            desc->ctrls->ax_options = SetClear (manager->settings->get("slowkeys-beep-press").toBool(),
                                                 desc->ctrls->ax_options,
                                                 XkbAccessXFeedbackMask | XkbAX_SKPressFBMask);
            desc->ctrls->ax_options = SetClear (manager->settings->get("slowkeys-beep-accept").toBool(),
                                                 desc->ctrls->ax_options,
                                                 XkbAccessXFeedbackMask | XkbAX_SKAcceptFBMask);
            desc->ctrls->ax_options = SetClear (manager->settings->get("slowkeys-beep-reject").toBool(),
                                                 desc->ctrls->ax_options,
                                                 XkbAccessXFeedbackMask | XkbAX_SKRejectFBMask);
            desc->ctrls->slow_keys_delay = GetInt (manager->settings,
                                                    "slowkeys-delay");
            /* anything larger than 500 seems to loose all keyboard input */
            if (desc->ctrls->slow_keys_delay > 500)
                    desc->ctrls->slow_keys_delay = 500;
    }

    /* sticky keys */
    if (SetCtrlFromSettings (desc,
                             manager->settings,
                             "stickykeys-enable",
                             XkbStickyKeysMask)) {
            desc->ctrls->ax_options |= XkbAX_LatchToLockMask;
            desc->ctrls->ax_options = SetClear (manager->settings->get("stickykeys-two-key-off").toBool(),
                                                 desc->ctrls->ax_options,
                                                 XkbAccessXFeedbackMask | XkbAX_TwoKeysMask);
            desc->ctrls->ax_options = SetClear (manager->settings->get("stickykeys-modifier-beep").toBool(),
                                                 desc->ctrls->ax_options,
                                                 XkbAccessXFeedbackMask | XkbAX_StickyKeysFBMask);
    }

    /* toggle keys */
    desc->ctrls->ax_options = SetClear (manager->settings->get("togglekeys-enable").toBool(),
                                         desc->ctrls->ax_options,
                                         XkbAccessXFeedbackMask | XkbAX_IndicatorFBMask);

    /*
    qDebug ("CHANGE to : 0x%x", desc->ctrls->enabled_ctrls);
    qDebug ("CHANGE to : 0x%x (2)", desc->ctrls->ax_options);
    */
    gdk_x11_display_error_trap_push (gdk_display_get_default());
    XkbSetControls (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                    XkbSlowKeysMask         |
                    XkbBounceKeysMask       |
                    XkbStickyKeysMask       |
                    XkbMouseKeysMask        |
                    XkbMouseKeysAccelMask   |
                    XkbAccessXKeysMask      |
                    XkbAccessXTimeoutMask   |
                    XkbAccessXFeedbackMask  |
                    XkbControlsEnabledMask,
                    desc);

    XkbFreeKeyboard (desc, XkbAllComponentsMask, True);

    XSync (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), false);
    gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default());
}

void A11yKeyboardManager::OnPreferencesDialogResponse(A11yKeyboardManager *manager)
{
    manager->preferences_dialog->close();
    delete manager->preferences_dialog;
}

void A11yKeyboardManager::OnStatusIconActivate(GtkStatusIcon *status_icon, A11yKeyboardManager *manager)
{
    if (manager->preferences_dialog == NULL) {
            manager->preferences_dialog = new A11yPreferencesDialog();
            connect(manager->preferences_dialog,SIGNAL(response(A11yKeyboardManager)),manager,SLOT(OnPreferencesDialogResponse(A11yKeyboardManager)));
            manager->preferences_dialog->show();
    } else {
            manager->preferences_dialog->close();
            delete manager->preferences_dialog;
            }
}

void A11yKeyboardManager::A11yKeyboardManagerEnsureStatusIcon(A11yKeyboardManager *manager)
{
    /*if (!manager->status_icon) {
        manager->status_icon = gtk_status_icon_new_from_icon_name ("preferences-desktop-accessibility");
        g_signal_connect (manager->status_icon,
                      "activate",
                      G_CALLBACK (OnStatusIconActivate),
                      manager);
    }*/
}

void A11yKeyboardManager::MaybeShowStatusIcon(A11yKeyboardManager *manager)
{
    bool     show;
    /* for now, show if accessx is enabled */
    show = manager->settings->get("enable").toBool();

   /* if (!show && manager->status_icon == NULL)
            return;
    */
    A11yKeyboardManagerEnsureStatusIcon (manager);
    //gtk_status_icon_set_visible (manager->status_icon, show);
}

void A11yKeyboardManager::KeyboardCallback(QString key)
{
    SetServerFromSettings (this);
    //MaybeShowStatusIcon (this);
}

bool A11yKeyboardManager::XkbEnabled (A11yKeyboardManager *manager)
{
        bool have_xkb;
        int opcode, errorBase, major, minor;

        have_xkb = XkbQueryExtension (gdk_x11_get_default_xdisplay(),
                                      &opcode,
                                      &manager->xkbEventBase,
                                      &errorBase,
                                      &major,
                                      &minor)
                && XkbUseExtension (gdk_x11_get_default_xdisplay(), &major, &minor);

        return have_xkb;
}

static bool SupportsXinputDevices (void)
{
        int op_code, event, error;

        return XQueryExtension (gdk_x11_get_default_xdisplay(),
                                "XInputExtension",
                                &op_code,
                                &event,
                                &error);
}

static GdkFilterReturn
DevicepresenceFilter (GdkXEvent *xevent,
                      GdkEvent  *event,
                      gpointer   data)
{
    XEvent *xev = (XEvent *) xevent;
    XEventClass class_presence;
    int xi_presence;
    A11yKeyboardManager *manager = (A11yKeyboardManager *)data;
    DevicePresence (gdk_x11_get_default_xdisplay (), xi_presence, class_presence);

    if (xev->type == xi_presence)
    {
        XDevicePresenceNotifyEvent *dpn = (XDevicePresenceNotifyEvent *) xev;
        if (dpn->devchange == DeviceEnabled) {
            manager->SetServerFromSettings (manager);
        }
    }
    return GDK_FILTER_CONTINUE;
}

void A11yKeyboardManager::SetDevicepresenceHandler (A11yKeyboardManager *manager)
{
    Display *display;
    XEventClass class_presence;
    int xi_presence;

    if (!SupportsXinputDevices ())
            return;

    display = gdk_x11_get_default_xdisplay ();

    gdk_x11_display_error_trap_push(gdk_display_get_default());
    DevicePresence (display, xi_presence, class_presence);
    /* FIXME:
     * Note that this might overwrite other events, see:
     * https://bugzilla.gnome.org/show_bug.cgi?id=610245#c2
     **/
    XSelectExtensionEvent (display,
                           RootWindow (display, DefaultScreen (display)),
                           &class_presence, 1);

    gdk_display_flush(gdk_display_get_default());
    if (!gdk_x11_display_error_trap_pop(gdk_display_get_default()))
            gdk_window_add_filter (NULL, DevicepresenceFilter, manager);
}

GdkFilterReturn CbXkbEventFilter (GdkXEvent              *xevent,
                                  GdkEvent               *ignored1,
                                  A11yKeyboardManager    *manager)
{
    XEvent   *xev   = (XEvent *) xevent;
    XkbEvent *xkbEv = (XkbEvent *) xevent;

    if (xev->xany.type == (manager->xkbEventBase + XkbEventCode) &&
        xkbEv->any.xkb_type == XkbControlsNotify) {
            qDebug ("XKB state changed");
            manager->SetSettingsFromServer (manager);
    } else if (xev->xany.type == (manager->xkbEventBase + XkbEventCode) &&
               xkbEv->any.xkb_type == XkbAccessXNotify) {
            if (xkbEv->accessx.detail == XkbAXN_AXKWarning) {
                    qDebug ("About to turn on an AccessX feature from the keyboard!");
                    /*
                     * TODO: when XkbAXN_AXKWarnings start working, we need to
                     * invoke ax_keys_warning_dialog_run here instead of in
                     * SetSettingsFromServer().
                     */
            }
    }

    return GDK_FILTER_CONTINUE;
}

bool A11yKeyboardManager::AxResponseCallback(A11yKeyboardManager *manager,
                                             QMessageBox         *parent,
                                             int                  response_id,
                                             unsigned int revert_controls_mask,
                                             bool                 enabled)
{
    switch (response_id) {
    case GTK_RESPONSE_DELETE_EVENT:
    case GTK_RESPONSE_REJECT:
    case GTK_RESPONSE_CANCEL:

        /* we're reverting, so we invert sense of 'enabled' flag */
        qDebug ("cancelling AccessX request");
        if (revert_controls_mask == XkbStickyKeysMask) {
            manager->settings->set("stickykeys-enable", !enabled);
        }
        else if (revert_controls_mask == XkbSlowKeysMask) {
            manager->settings->set("slowkeys-enable",!enabled);
        }
        manager->SetServerFromSettings (manager);
        break;
    case GTK_RESPONSE_HELP:
        if(!parent->isActiveWindow())
        {
            QMessageBox *error_dialog = new QMessageBox();
            error_dialog->warning(nullptr,nullptr,tr("There was an error displaying help"),QMessageBox::Close);
            error_dialog->setResult(false);
            error_dialog->show();
        }
        return FALSE;
    default:
        break;
    }
return TRUE;
}


#ifdef HAVE_LIBNOTIFY
void OnNotificationClosed (NotifyNotification     *notification,
                           A11yKeyboardManager    *manager)
{
        g_object_unref (manager->notification);
        manager->notification = NULL;
}

void
on_sticky_keys_action (NotifyNotification     *notification,
                       const char             *action,
                       A11yKeyboardManager    *manager)
{
    bool res;
    int      response_id;

    g_assert (action != NULL);

    if (strcmp (action, "accept") == 0) {
            response_id = GTK_RESPONSE_ACCEPT;
    } else if (strcmp (action, "reject") == 0) {
            response_id = GTK_RESPONSE_REJECT;
    } else {
            return;
    }

    res = manager->AxResponseCallback (manager, nullptr,
                                       response_id, XkbStickyKeysMask,
                                       manager->StickykeysShortcutVal);
    if (res) {
            notify_notification_close (manager->notification, NULL);
    }
}

void
on_slow_keys_action (NotifyNotification     *notification,
                     const char             *action,
                     A11yKeyboardManager    *manager)
{
        bool res;
        int      response_id;

        g_assert (action != NULL);

        if (strcmp (action, "accept") == 0) {
                response_id = GTK_RESPONSE_ACCEPT;
        } else if (strcmp (action, "reject") == 0) {
                response_id = GTK_RESPONSE_REJECT;
        } else {
                return;
        }

        res = manager->AxResponseCallback (manager, nullptr,
                                    response_id, XkbSlowKeysMask,
                                    manager->SlowkeysShortcutVal);
        if (res) {
                notify_notification_close (manager->notification, NULL);
        }
}
#endif /* HAVE_LIBNOTIFY */

bool AxSlowkeysWarningPostDubble (A11yKeyboardManager *manager,
                                      bool                 enabled)
{
#ifdef HAVE_LIBNOTIFY
    bool        res;
    QString     title;
    QString     message;
    GError      *error;

    title = enabled ?
             QObject::tr("Do you want to activate Slow Keys?") :
             QObject::tr("Do you want to deactivate Slow Keys?");

    message =  QObject::tr("You just held down the Shift key for 8 seconds.  This is the shortcut "
                "for the Slow Keys feature, which affects the way your keyboard works.");

   /* if (manager->status_icon == NULL || ! gtk_status_icon_is_embedded (manager->status_icon)) {
            return FALSE;
    }
    */

    if (manager->SlowkeysAlert != NULL) {
        manager->SlowkeysAlert->close();
        delete manager->SlowkeysAlert;
    }

    if (manager->notification != NULL) {
        notify_notification_close (manager->notification, NULL);
    }

    manager->A11yKeyboardManagerEnsureStatusIcon (manager);
    manager->notification = notify_notification_new (title.toLatin1().data(),
                                                     message.toLatin1().data(),
                                                     "preferences-desktop-accessibility");
    notify_notification_set_timeout (manager->notification, NOTIFICATION_TIMEOUT * 1000);

    notify_notification_add_action (manager->notification,
                                    "reject",
                                    enabled ? _("Don't activate") : _("Don't deactivate"),
                                    (NotifyActionCallback) on_slow_keys_action,
                                    manager,
                                    NULL);
    notify_notification_add_action (manager->notification,
                                    "accept",
                                    enabled ? _("Activate") : _("Deactivate"),
                                    (NotifyActionCallback) on_slow_keys_action,
                                    manager,
                                    NULL);

    g_signal_connect (manager->notification,
                      "closed",
                      G_CALLBACK (OnNotificationClosed),
                      manager);

    error = NULL;
    res = notify_notification_show (manager->notification, &error);
    if (! res) {
            g_warning ("UsdA11yKeyboardManager: unable to show notification: %s", error->message);
            g_error_free (error);
            notify_notification_close (manager->notification, NULL);
    }

    return res;
#else
    return false;
#endif
}

void A11yKeyboardManager::ax_slowkeys_response (QAbstractButton *button)
{
    int response_id = 0;
    if(button == (QAbstractButton *)QMessageBox::Help)
        response_id = GTK_RESPONSE_HELP;
    else if(button == (QAbstractButton *)QMessageBox::Cancel)
        response_id = GTK_RESPONSE_CANCEL;
    if (AxResponseCallback (this, StickykeysAlert,
                            response_id, XkbSlowKeysMask,
                            SlowkeysShortcutVal)) {
        StickykeysAlert->close();
    }
}

void A11yKeyboardManager::AxSlowkeysWarningPostDialog (A11yKeyboardManager *manager,
                                                       bool                enabled)
{
        QString title;
        QString message;

        title = enabled ?
                tr("Do you want to activate Slow Keys?") :
                tr("Do you want to deactivate Slow Keys?");
        message = tr("You just held down the Shift key for 8 seconds.  This is the shortcut "
                    "for the Slow Keys feature, which affects the way your keyboard works.");

        if (manager->SlowkeysAlert != NULL) {
                manager->SlowkeysAlert->show();
                return;
        }

        manager->SlowkeysAlert = new QMessageBox();
        manager->SlowkeysAlert->warning(nullptr,tr("Slow Keys Alert"), title);
        manager->SlowkeysAlert->setText(message);
        manager->SlowkeysAlert->setStandardButtons(QMessageBox::Help);
        manager->SlowkeysAlert->setButtonText(QMessageBox::Rejected,
                                                enabled ? tr("Do_n't activate") : tr("Do_n't deactivate"));
        manager->SlowkeysAlert->setButtonText(QMessageBox::Accepted,
                                                enabled ? tr("_Activate") : tr("_Deactivate"));

        manager->SlowkeysAlert->setWindowIconText(tr("input-keyboard"));

        manager->SlowkeysAlert->setDefaultButton(QMessageBox::Default);

        QObject::connect(manager->SlowkeysAlert,SIGNAL(buttonClicked(QAbstractButton *button)),
                         manager,SLOT(ax_slowkeys_response(QAbstractButton *button)));

        manager->SlowkeysAlert->show();

        /*g_object_add_weak_pointer (G_OBJECT (manager->StickykeysAlert),
                                   (gpointer*) &manager->StickykeysAlert);*/
}

void A11yKeyboardManager::AxSlowkeysWarningPost (A11yKeyboardManager *manager,
                                                 bool                 enabled)
{

        manager->SlowkeysShortcutVal = enabled;

        /* alway try to show something */
        if (! AxSlowkeysWarningPostDubble (manager, enabled)) {
                AxSlowkeysWarningPostDialog (manager, enabled);
        }
}


bool AxStickykeysWarningPostBubble (A11yKeyboardManager *manager,
                                    bool                enabled)
{
#ifdef HAVE_LIBNOTIFY
    bool    res;
    QString title;
    QString message;
    GError  *error;

    title = enabled ?
            QObject::tr("Do you want to activate Sticky Keys?") :
            QObject::tr("Do you want to deactivate Sticky Keys?");
    message = enabled ?
            QObject::tr("You just pressed the Shift key 5 times in a row.  This is the shortcut "
              "for the Sticky Keys feature, which affects the way your keyboard works.") :
            QObject::tr("You just pressed two keys at once, or pressed the Shift key 5 times in a row.  "
              "This turns off the Sticky Keys feature, which affects the way your keyboard works.");


    if (manager->SlowkeysAlert != NULL) {
            manager->SlowkeysAlert->close();
            delete manager->SlowkeysAlert;
    }

    if (manager->notification != NULL) {
            notify_notification_close (manager->notification, NULL);
    }

    //usd_a11y_keyboard_manager_ensure_status_icon (manager);
    manager->notification = notify_notification_new (title.toLatin1().data(),
                                                     message.toLatin1().data(),
                                                     "preferences-desktop-accessibility");
    notify_notification_set_timeout (manager->notification, NOTIFICATION_TIMEOUT * 1000);

    notify_notification_add_action (manager->notification,
                                    "reject",
                                    enabled ? _("Don't activate") : _("Don't deactivate"),
                                    (NotifyActionCallback) on_sticky_keys_action,
                                    manager,
                                    NULL);
    notify_notification_add_action (manager->notification,
                                    "accept",
                                    enabled ? _("Activate") : _("Deactivate"),
                                    (NotifyActionCallback) on_sticky_keys_action,
                                    manager,
                                    NULL);

    g_signal_connect (manager->notification,
                      "closed",
                      G_CALLBACK (OnNotificationClosed),
                      manager);

    error = NULL;
    res = notify_notification_show (manager->notification, &error);
    if (! res) {
            qWarning ("UsdA11yKeyboardManager: unable to show notification: %s", error->message);
            g_error_free (error);
            notify_notification_close (manager->notification, NULL);
    }

    return res;
#else
    return FALSE;
#endif /* HAVE_LIBNOTIFY */
}

void A11yKeyboardManager::ax_stickykeys_response(QAbstractButton *button)
{
    int response_id = 0;
    if(button == (QAbstractButton *)QMessageBox::Help)
        response_id = GTK_RESPONSE_HELP;
    else if(button == (QAbstractButton *)QMessageBox::Cancel)
        response_id = GTK_RESPONSE_CANCEL;
    if(AxResponseCallback(this,StickykeysAlert, response_id,XkbStickyKeysMask,StickykeysShortcutVal))
        StickykeysAlert->close();
}

void A11yKeyboardManager::AxStickykeysWarningPostDialog (A11yKeyboardManager *manager,
                                                         bool             enabled)
{
    QString  title;
    QString  message;

    title = enabled ?
            tr("Do you want to activate Sticky Keys?") :
            tr("Do you want to deactivate Sticky Keys?");
    message = enabled ?
            tr("You just pressed the Shift key 5 times in a row.  This is the shortcut "
              "for the Sticky Keys feature, which affects the way your keyboard works.") :
            tr("You just pressed two keys at once, or pressed the Shift key 5 times in a row.  "
              "This turns off the Sticky Keys feature, which affects the way your keyboard works.");

    if (manager->StickykeysAlert != NULL) {
            manager->StickykeysAlert->show();
            return;
    }

    manager->StickykeysAlert = new QMessageBox();
    manager->StickykeysAlert->warning(nullptr,tr("Sticky Keys Alert"), title);
    manager->StickykeysAlert->setText(message);
    manager->StickykeysAlert->setStandardButtons(QMessageBox::Help);
    manager->StickykeysAlert->setButtonText(QMessageBox::Rejected,
                                            enabled ? tr("Do_n't activate") : tr("Do_n't deactivate"));
    manager->StickykeysAlert->setButtonText(QMessageBox::Accepted,
                                            enabled ? tr("_Activate") : tr("_Deactivate"));
    manager->StickykeysAlert->setWindowIconText(tr("input-keyboard"));

    manager->StickykeysAlert->setDefaultButton(QMessageBox::Default);

    QObject::connect(manager->StickykeysAlert,SIGNAL(buttonClicked(QAbstractButton *button)),
                     manager,SLOT(ax_stickykeys_response(QAbstractButton *button)));

    manager->StickykeysAlert->show();

    /*g_object_add_weak_pointer (G_OBJECT (manager->stickykeys_alert),
                               (gpointer*) &manager->stickykeys_alert);*/
}

void A11yKeyboardManager::AxStickykeysWarningPost (A11yKeyboardManager *manager,
                                                   bool                enabled)
{
        manager->StickykeysShortcutVal = enabled;
        /* alway try to show something */
        if (! AxStickykeysWarningPostBubble (manager, enabled)) {
                AxStickykeysWarningPostDialog (manager, enabled);
        }
}


void A11yKeyboardManager::SetSettingsFromServer(A11yKeyboardManager *manager)
{
    QGSettings      *settings;
    XkbDescRec     *desc;
    bool        changed = false;
    bool        slowkeys_changed;
    bool        stickykeys_changed;

    desc = GetXkbDescRec ();
    if (! desc) {
            return;
    }
    settings = new QGSettings(CONFIG_SCHEMA);
    settings->delay();
    /*
      fprintf (stderr, "changed to : 0x%x\n", desc->ctrls->enabled_ctrls);
      fprintf (stderr, "changed to : 0x%x (2)\n", desc->ctrls->ax_options);
    */
    changed |= SetBool (settings,
                        "enable",
                        desc->ctrls->enabled_ctrls & XkbAccessXKeysMask);

    changed |= SetBool (settings,
                        "feature-state-change-beep",
                        desc->ctrls->ax_options & (XkbAX_FeatureFBMask | XkbAX_SlowWarnFBMask));
    changed |= SetBool (settings,
                        "timeout-enable",
                        desc->ctrls->enabled_ctrls & XkbAccessXTimeoutMask);
    changed |= SetInt (settings,
                        "timeout",
                        desc->ctrls->ax_timeout);

    changed |= SetBool (settings,
                        "bouncekeys-enable",
                        desc->ctrls->enabled_ctrls & XkbBounceKeysMask);
    changed |= SetInt (settings,
                        "bouncekeys-delay",
                        desc->ctrls->debounce_delay);
    changed |= SetBool (settings,
                        "bouncekeys-beep-reject",
                        desc->ctrls->ax_options & XkbAX_BKRejectFBMask);

    changed |= SetBool (settings,
                        "mousekeys-enable",
                        desc->ctrls->enabled_ctrls & XkbMouseKeysMask);
    changed |= SetInt (settings,
                        "mousekeys-max-speed",
                        desc->ctrls->mk_max_speed * (1000 / desc->ctrls->mk_interval));
    /* NOTE : mk_time_to_max is measured in events not time */
    changed |= SetInt (settings,
                        "mousekeys-accel-time",
                        desc->ctrls->mk_time_to_max * desc->ctrls->mk_interval);
    changed |= SetInt (settings,
                        "mousekeys-init-delay",
                        desc->ctrls->mk_delay);

    slowkeys_changed = SetBool (settings,
                                 "slowkeys-enable",
                                 desc->ctrls->enabled_ctrls & XkbSlowKeysMask);
    changed |= SetBool (settings,
                         "slowkeys-beep-press",
                         desc->ctrls->ax_options & XkbAX_SKPressFBMask);
    changed |= SetBool (settings,
                         "slowkeys-beep-accept",
                         desc->ctrls->ax_options & XkbAX_SKAcceptFBMask);
    changed |= SetBool (settings,
                         "slowkeys-beep-reject",
                         desc->ctrls->ax_options & XkbAX_SKRejectFBMask);
    changed |= SetInt (settings,
                        "slowkeys-delay",
                        desc->ctrls->slow_keys_delay);

    stickykeys_changed = SetBool (settings,
                                   "stickykeys-enable",
                                   desc->ctrls->enabled_ctrls & XkbStickyKeysMask);
    changed |= SetBool (settings,
                         "stickykeys-two-key-off",
                         desc->ctrls->ax_options & XkbAX_TwoKeysMask);
    changed |= SetBool (settings,
                         "stickykeys-modifier-beep",
                         desc->ctrls->ax_options & XkbAX_StickyKeysFBMask);

    changed |= SetBool (settings,
                         "togglekeys-enable",
                         desc->ctrls->ax_options & XkbAX_IndicatorFBMask);

    if (!changed && stickykeys_changed ^ slowkeys_changed) {
            /*
             * sticky or slowkeys has changed, singly, without our intervention.
             * 99% chance this is due to a keyboard shortcut being used.
             * we need to detect via this hack until we get
             *  XkbAXN_AXKWarning notifications working (probable XKB bug),
             *  at which time we can directly intercept such shortcuts instead.
             * See CbXkbEventFilter () below.
             */

            /* sanity check: are keyboard shortcuts available? */
            if (desc->ctrls->enabled_ctrls & XkbAccessXKeysMask) {
                    if (slowkeys_changed) {
                            AxSlowkeysWarningPost (manager,
                                                      desc->ctrls->enabled_ctrls & XkbSlowKeysMask);
                    } else {
                            AxStickykeysWarningPost (manager,
                                                        desc->ctrls->enabled_ctrls & XkbStickyKeysMask);
                    }
            }
    }

    XkbFreeKeyboard (desc, XkbAllComponentsMask, True);

    changed |= (stickykeys_changed | slowkeys_changed);

    if (changed) {
            settings->apply();
    }

    delete settings;
}

void A11yKeyboardManager::StartA11yKeyboardIdleCb()
{
    unsigned int event_mask;

    qDebug("Starting a11y_keyboard manager");

    time->stop();

    if (!XkbEnabled (this))
        goto out;

    connect(settings,SIGNAL(changed(QString)),this,SLOT(KeyboardCallback(QString)));
    SetDevicepresenceHandler (this);

    /* Save current xkb state so we can restore it on exit
     */
    OriginalXkbDesc = GetXkbDescRec ();

    event_mask = XkbControlsNotifyMask;
#ifdef DEBUG_ACCESSIBILITY
    event_mask |= XkbAccessXNotifyMask; /* make default when AXN_AXKWarning works */
#endif

    /* be sure to init before starting to monitor the server */
    SetServerFromSettings (this);

    XkbSelectEvents (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                     XkbUseCoreKbd,
                     event_mask,
                     event_mask);

    gdk_window_add_filter (NULL,
                           (GdkFilterFunc) CbXkbEventFilter,
                           this);

    //MaybeShowStatusIcon (this);

out:
    return;
}
