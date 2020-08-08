#include "a11y-keyboard-manager.h"
#include "clib-syslog.h"
#include "config.h"

#define CONFIG_SCHEMA "org.mate.accessibility-keyboard"
#define NOTIFICATION_TIMEOUT 30
#undef DEBUG_ACCESSIBILITY
#ifdef DEBUG_ACCESSIBILITY
#define d(str)          g_debug (str)
#else
#define d(str)          do { } while (0)
#endif
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
   CT_SYSLOG(LOG_DEBUG,"Stoping A11y Keyboard manager");
   gdk_window_remove_filter (NULL, DevicepresenceFilter, this);

    if (status_icon)
       gtk_status_icon_set_visible (status_icon, FALSE);

    gdk_window_remove_filter (NULL,
                         (GdkFilterFunc) CbXkbEventFilter,
                         this);

    /* Disable all the AccessX bits
    */
    restore_server_xkb_config (this);
    if (slowkeys_alert != NULL)
       gtk_widget_destroy (slowkeys_alert);

    if (stickykeys_alert != NULL)
       gtk_widget_destroy (stickykeys_alert);

    slowkeys_shortcut_val = FALSE;
    stickykeys_shortcut_val = FALSE;

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

static gboolean
set_int (QGSettings      *settings,
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

void A11yKeyboardManager::restore_server_xkb_config (A11yKeyboardManager *manager)
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
                        manager->original_xkb_desc);

        XkbFreeKeyboard (manager->original_xkb_desc,
                         XkbAllComponentsMask, True);

        XSync (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), FALSE);
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default());

        manager->original_xkb_desc = NULL;
}

XkbDescRec * A11yKeyboardManager::GetXkbDescRec()
{
        XkbDescRec *desc;
        Status      status = Success;

        gdk_x11_display_error_trap_push (gdk_display_get_default());
        desc = XkbGetMap (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), XkbAllMapComponentsMask, XkbUseCoreKbd);
        if (desc != NULL) {
                desc->ctrls = NULL;
                status = XkbGetControls (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), XkbAllControlsMask, desc);
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

    XSync (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), FALSE);
    gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default());
}

void A11yKeyboardManager::on_status_icon_activate(GtkStatusIcon *status_icon, A11yKeyboardManager *manager)
{
    // 等待图形界面 Wait graphical interface
    /*
    if (manager->preferences_dialog == NULL) {
            manager->preferences_dialog = usd_a11y_preferences_dialog_new ();
            g_signal_connect (manager->preferences_dialog,
                              "response",
                              G_CALLBACK (on_preferences_dialog_response),
                              manager);
            gtk_window_present (GTK_WINDOW (manager->preferences_dialog));
    } else {
            g_signal_handlers_disconnect_by_func (manager->preferences_dialog,
                                                  on_preferences_dialog_response,
                                                  manager);
            gtk_widget_destroy (GTK_WIDGET (manager->preferences_dialog));
            manager->preferences_dialog = NULL;
            }
    }
    */
}
void A11yKeyboardManager::usd_a11y_keyboard_manager_ensure_status_icon(A11yKeyboardManager *manager)
{
    if (!manager->status_icon) {
        manager->status_icon = gtk_status_icon_new_from_icon_name ("preferences-desktop-accessibility");
        g_signal_connect (manager->status_icon,
                          "activate",
                          G_CALLBACK (on_status_icon_activate),
                          manager);
    }
}

void A11yKeyboardManager::maybe_show_status_icon(A11yKeyboardManager *manager)
{
    bool     show;
    /* for now, show if accessx is enabled */
    show = manager->settings->get("enable").toBool();

    if (!show && manager->status_icon == NULL)
            return;

    usd_a11y_keyboard_manager_ensure_status_icon (manager);
    gtk_status_icon_set_visible (manager->status_icon, show);
}

void A11yKeyboardManager::KeyboardCallback(QString key)
{
    SetServerFromSettings (this);
    maybe_show_status_icon (this);
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
                d ("XKB state changed");
                manager->set_settings_from_server (manager);
        } else if (xev->xany.type == (manager->xkbEventBase + XkbEventCode) &&
                   xkbEv->any.xkb_type == XkbAccessXNotify) {
                if (xkbEv->accessx.detail == XkbAXN_AXKWarning) {
                        d ("About to turn on an AccessX feature from the keyboard!");
                        /*
                         * TODO: when XkbAXN_AXKWarnings start working, we need to
                         * invoke ax_keys_warning_dialog_run here instead of in
                         * set_settings_from_server().
                         */
                }
        }

        return GDK_FILTER_CONTINUE;
}

void ax_slowkeys_warning_post (A11yKeyboardManager *manager,
                               gboolean             enabled)
{

        manager->slowkeys_shortcut_val = enabled;

        /* alway try to show something */
        /*if (! ax_slowkeys_warning_post_bubble (manager, enabled)) {
                ax_slowkeys_warning_post_dialog (manager, enabled);
        }*/
}
void ax_stickykeys_warning_post (A11yKeyboardManager *manager,
                            gboolean                enabled)
{
        manager->stickykeys_shortcut_val = enabled;
        /* alway try to show something */
        /*if (! ax_stickykeys_warning_post_bubble (manager, enabled)) {
                ax_stickykeys_warning_post_dialog (manager, enabled);
        }*/
}


void A11yKeyboardManager::set_settings_from_server(A11yKeyboardManager *manager)
{
    QGSettings      *settings;
    XkbDescRec     *desc;
    gboolean        changed = FALSE;
    gboolean        slowkeys_changed;
    gboolean        stickykeys_changed;

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
    changed |= set_int (settings,
                        "timeout",
                        desc->ctrls->ax_timeout);

    changed |= SetBool (settings,
                         "bouncekeys-enable",
                         desc->ctrls->enabled_ctrls & XkbBounceKeysMask);
    changed |= set_int (settings,
                        "bouncekeys-delay",
                        desc->ctrls->debounce_delay);
    changed |= SetBool (settings,
                         "bouncekeys-beep-reject",
                         desc->ctrls->ax_options & XkbAX_BKRejectFBMask);

    changed |= SetBool (settings,
                         "mousekeys-enable",
                         desc->ctrls->enabled_ctrls & XkbMouseKeysMask);
    changed |= set_int (settings,
                        "mousekeys-max-speed",
                        desc->ctrls->mk_max_speed * (1000 / desc->ctrls->mk_interval));
    /* NOTE : mk_time_to_max is measured in events not time */
    changed |= set_int (settings,
                        "mousekeys-accel-time",
                        desc->ctrls->mk_time_to_max * desc->ctrls->mk_interval);
    changed |= set_int (settings,
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
    changed |= set_int (settings,
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
                            ax_slowkeys_warning_post (manager,
                                                      desc->ctrls->enabled_ctrls & XkbSlowKeysMask);
                    } else {
                            ax_stickykeys_warning_post (manager,
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
    original_xkb_desc = GetXkbDescRec ();

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

    maybe_show_status_icon (this);

out:
    return;
}
