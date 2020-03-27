#include "keyboard-manager.h"
#include "clib-syslog.h"
#include "../../config.h"

gboolean start_keyboard_idle_cb (KeyboardManager *manager);
GdkFilterReturn xkb_events_filter (GdkXEvent *xev_,
                                   GdkEvent  *gdkev_,
                                   gpointer   user_data);
static void numlock_set_xkb_state (NumLockState new_state);

KeyboardManager * KeyboardManager::mKeyboardManager = nullptr;

KeyboardManager::KeyboardManager(QObject * parent)
{
    mKeyXkb = new KeyboardXkb;
}

KeyboardManager::~KeyboardManager()
{
    delete mKeyXkb;
}

KeyboardManager *KeyboardManager::KeyboardManagerNew()
{
    if (nullptr == mKeyboardManager)
        mKeyboardManager = new KeyboardManager(nullptr);
    return  mKeyboardManager;
}


bool KeyboardManager::KeyboardManagerStart()
{
    CT_SYSLOG(LOG_DEBUG,"-- Keyboard Start Manager --");

    g_idle_add ((GSourceFunc) start_keyboard_idle_cb, this);

    return true;
}

void KeyboardManager::KeyboardManagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"-- Keyboard Stop Manager --");

    if (this->settings != NULL) {
            g_object_unref (this->settings);
            this->settings = NULL;
    }
    this->old_state = 0;
    numlock_set_xkb_state((NumLockState)this->old_state);

#if HAVE_X11_EXTENSIONS_XKB_H
    if (this->have_xkb) {
        gdk_window_remove_filter (NULL,
                                  xkb_events_filter,
                                  GINT_TO_POINTER (this));
    }

#endif /* HAVE_X11_EXTENSIONS_XKB_H */

    mKeyXkb->usd_keyboard_xkb_shutdown ();
}


#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
static gboolean xfree86_set_keyboard_autorepeat_rate(int delay, int rate)
{
        gboolean res = FALSE;
        int      event_base_return;
        int      error_base_return;

        if (XF86MiscQueryExtension (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                                    &event_base_return,
                                    &error_base_return) == True) {
                /* load the current settings */
                XF86MiscKbdSettings kbdsettings;
                XF86MiscGetKbdSettings (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), &kbdsettings);

                /* assign the new values */
                kbdsettings.delay = delay;
                kbdsettings.rate = rate;
                XF86MiscSetKbdSettings (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), &kbdsettings);
                res = TRUE;
        }

        return res;
}
#endif /* HAVE_X11_EXTENSIONS_XF86MISC_H */

void numlock_xkb_init (KeyboardManager *manager)
{
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        gboolean have_xkb;
        int opcode, error_base, major, minor;

        have_xkb = XkbQueryExtension (dpy,
                                      &opcode,
                                      &manager->xkb_event_base,
                                      &error_base,
                                      &major,
                                      &minor)
                && XkbUseExtension (dpy, &major, &minor);

        if (have_xkb) {
                XkbSelectEventDetails (dpy,
                                       XkbUseCoreKbd,
                                       XkbStateNotifyMask,
                                       XkbModifierLockMask,
                                       XkbModifierLockMask);
        } else {
                g_warning ("XKB extension not available");
        }

        manager->have_xkb = have_xkb;
}

static NumLockState numlock_get_settings_state (GSettings *settings)
{
        int          curr_state;
        curr_state = g_settings_get_enum (settings, KEY_NUMLOCK_STATE);
        return (NumLockState)curr_state;
}

static unsigned numlock_NumLock_modifier_mask (void)
{
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        return XkbKeysymToModifiers (dpy, XK_Num_Lock);
}

static void numlock_set_xkb_state (NumLockState new_state)
{
        unsigned int num_mask;
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        if (new_state != NUMLOCK_STATE_ON && new_state != NUMLOCK_STATE_OFF)
                return;
        num_mask = numlock_NumLock_modifier_mask ();
        XkbLockModifiers (dpy, XkbUseCoreKbd, num_mask, new_state ? num_mask : 0);
}

void apply_bell (KeyboardManager *manager)
{
        GSettings       *settings;
        XKeyboardControl kbdcontrol;
        gboolean         click;
        int              bell_volume;
        int              bell_pitch;
        int              bell_duration;
        char		*volume_string;
        int              click_volume;

        g_debug ("Applying the bell settings");
        settings      = manager->settings;
        click         = g_settings_get_boolean  (settings, KEY_CLICK);
        click_volume  = g_settings_get_int   (settings, KEY_CLICK_VOLUME);

        bell_pitch    = g_settings_get_int   (settings, KEY_BELL_PITCH);
        bell_duration = g_settings_get_int   (settings, KEY_BELL_DURATION);

        volume_string = g_settings_get_string (settings, KEY_BELL_MODE);
        bell_volume   = (volume_string && !strcmp (volume_string, "on")) ? 50 : 0;
        g_free (volume_string);

        /* as percentage from 0..100 inclusive */
        if (click_volume < 0) {
                click_volume = 0;
        } else if (click_volume > 100) {
                click_volume = 100;
        }
        kbdcontrol.key_click_percent = click ? click_volume : 0;
        kbdcontrol.bell_percent = bell_volume;
        kbdcontrol.bell_pitch = bell_pitch;
        kbdcontrol.bell_duration = bell_duration;
        gdk_error_trap_push ();
        XChangeKeyboardControl (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                KBKeyClickPercent | KBBellPercent | KBBellPitch | KBBellDuration,
                                &kbdcontrol);

        XSync (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
        //gdk_error_trap_pop_ignored ();
}

void apply_numlock (KeyboardManager *manager)
{
        GSettings *settings;
        gboolean rnumlock;

        g_debug ("Applying the num-lock settings");
        settings = manager->settings;
        rnumlock = g_settings_get_boolean  (settings, KEY_NUMLOCK_REMEMBER);
        manager->old_state = g_settings_get_enum (manager->settings, KEY_NUMLOCK_STATE);

        gdk_error_trap_push ();
        if (rnumlock) {
                numlock_set_xkb_state ((NumLockState)manager->old_state);
        }

        XSync (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
        //gdk_error_trap_pop_ignored ();
}

static gboolean xkb_set_keyboard_autorepeat_rate(int delay, int rate)
{
    int interval = (rate <= 0) ? 1000000 : 1000/rate;

    if (delay <= 0)
    {
        delay = 1;
    }

    return XkbSetAutoRepeatRate(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), XkbUseCoreKbd, delay, interval);
}

void apply_repeat (KeyboardManager *manager)
{
        GSettings       *settings;
        gboolean         repeat;
        guint            rate;
        guint            delay;

        g_debug ("Applying the repeat settings");
        settings      = manager->settings;
        repeat        = g_settings_get_boolean  (settings, KEY_REPEAT);
        rate	      = g_settings_get_int  (settings, KEY_RATE);
        delay         = g_settings_get_int  (settings, KEY_DELAY);

        gdk_error_trap_push ();
        if (repeat) {
                gboolean rate_set = FALSE;

                XAutoRepeatOn (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
                /* Use XKB in preference */
                rate_set = xkb_set_keyboard_autorepeat_rate (delay, rate);

                if (!rate_set)
                        g_warning ("Neither XKeyboard not Xfree86's keyboard extensions are available,\n"
                                   "no way to support keyboard autorepeat rate settings");
        } else {
                XAutoRepeatOff (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
        }

        XSync (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
        //gdk_error_trap_pop_ignored ();
}

void apply_settings (GSettings          *settings,
                    gchar              *key,
                    KeyboardManager *manager)
{
    /**
     * Fix by HB* system reboot but rnumlock not available;
    **/
#ifdef HAVE_X11_EXTENSIONS_XKB_H
    gboolean rnumlock;
    rnumlock = g_settings_get_boolean (settings, KEY_NUMLOCK_REMEMBER);

    if (rnumlock == 0 || key == NULL) {
        if (manager->have_xkb && rnumlock) {
            numlock_set_xkb_state (numlock_get_settings_state (settings));
        }
    }
#endif /* HAVE_X11_EXTENSIONS_XKB_H */
    if (g_strcmp0 (key, KEY_CLICK) == 0||
            g_strcmp0 (key, KEY_CLICK_VOLUME) == 0 ||
            g_strcmp0 (key, KEY_BELL_PITCH) == 0 ||
            g_strcmp0 (key, KEY_BELL_DURATION) == 0 ||
            g_strcmp0 (key, KEY_BELL_MODE) == 0) {
                g_debug ("Bell setting '%s' changed, applying bell settings", key);
                apply_bell (manager);
        } else if (g_strcmp0 (key, KEY_NUMLOCK_REMEMBER) == 0) {
                g_debug ("Remember Num-Lock state '%s' changed, applying num-lock settings", key);
                apply_numlock (manager);
        } else if (g_strcmp0 (key, KEY_NUMLOCK_STATE) == 0) {
                g_debug ("Num-Lock state '%s' changed, will apply at next startup", key);
        } else if (g_strcmp0 (key, KEY_REPEAT) == 0 ||
                 g_strcmp0 (key, KEY_RATE) == 0 ||
                 g_strcmp0 (key, KEY_DELAY) == 0) {
                g_debug ("Key repeat setting '%s' changed, applying key repeat settings", key);
                apply_repeat (manager);
        } else {
                g_warning ("Unhandled settings change, key '%s'", key);
        }
}

void KeyboardManager::usd_keyboard_manager_apply_settings (KeyboardManager *manager)
{
        apply_settings (manager->settings, NULL, manager);
}

GdkFilterReturn xkb_events_filter (GdkXEvent *xev_,
                                   GdkEvent  *gdkev_,
                                   gpointer   user_data)
{
        XEvent *xev = (XEvent *) xev_;
        XkbEvent *xkbev = (XkbEvent *) xev;
        KeyboardManager *manager = (KeyboardManager *) user_data;

        if (xev->type != manager->xkb_event_base ||
            xkbev->any.xkb_type != XkbStateNotify)
                return GDK_FILTER_CONTINUE;

        if (xkbev->state.changed & XkbModifierLockMask) {
                unsigned num_mask = numlock_NumLock_modifier_mask ();
                unsigned locked_mods = xkbev->state.locked_mods;
                int numlock_state;

                numlock_state = (num_mask & locked_mods) ? NUMLOCK_STATE_ON : NUMLOCK_STATE_OFF;

                if (numlock_state != manager->old_state) {
                        g_settings_set_enum (manager->settings,
                                             KEY_NUMLOCK_STATE,
                                             numlock_state);
                        manager->old_state = numlock_state;
                }
        }

        return GDK_FILTER_CONTINUE;
}

void numlock_install_xkb_callback (KeyboardManager *manager)
{
        if (!manager->have_xkb)
                return;

        gdk_window_add_filter (NULL,
                               xkb_events_filter,
                               GINT_TO_POINTER (manager));
}


gboolean start_keyboard_idle_cb (KeyboardManager *manager)
{
        g_debug ("Starting keyboard manager");

        manager->have_xkb = 0;
        manager->settings = g_settings_new (USD_KEYBOARD_SCHEMA);

        /* Essential - xkb initialization should happen before */
        manager->mKeyXkb->usd_keyboard_xkb_init (manager);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_xkb_init (manager);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        /* apply current settings before we install the callback */
        manager->usd_keyboard_manager_apply_settings (manager);

        g_signal_connect (manager->settings, "changed", G_CALLBACK (apply_settings), manager);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_install_xkb_callback (manager);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        return FALSE;
}
