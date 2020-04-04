#include "keyboard-manager.h"
#include "clib-syslog.h"
#include "config.h"
#include <QTimer>

GdkFilterReturn xkb_events_filter (GdkXEvent *xev_,
                                   GdkEvent  *gdkev_,
                                   gpointer   user_data);
static void numlock_set_xkb_state (NumLockState new_state);

KeyboardManager * KeyboardManager::mKeyboardManager = nullptr;
KeyboardXkb * KeyboardManager::mKeyXkb = nullptr;

KeyboardManager::KeyboardManager(QObject * parent)
{
    if(mKeyXkb == nullptr)
        mKeyXkb = new KeyboardXkb;
    settings = new QGSettings(USD_KEYBOARD_SCHEMA);
}

KeyboardManager::~KeyboardManager()
{
    delete mKeyXkb;
    delete settings;
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

    time = new QTimer(this);

    connect(time,SIGNAL(timeout()),this,SLOT(start_keyboard_idle_cb()));
    time->start();

    return true;
}

void KeyboardManager::KeyboardManagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"-- Keyboard Stop Manager --");

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
        Display *dpy = QX11Info::display();
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

static NumLockState numlock_get_settings_state (QGSettings *settings)
{
        int          curr_state;
        curr_state = settings->get(KEY_NUMLOCK_STATE).Int;
        return (NumLockState)curr_state;
}

static unsigned numlock_NumLock_modifier_mask (void)
{
        Display *dpy = QX11Info::display();
        return XkbKeysymToModifiers (dpy, XK_Num_Lock);
}

static void numlock_set_xkb_state (NumLockState new_state)
{
        unsigned int num_mask;
        Display *dpy = QX11Info::display();
        if (new_state != NUMLOCK_STATE_ON && new_state != NUMLOCK_STATE_OFF)
                return;
        num_mask = numlock_NumLock_modifier_mask ();
        XkbLockModifiers (dpy, XkbUseCoreKbd, num_mask, new_state ? num_mask : 0);
}

void apply_bell (KeyboardManager *manager)
{
        QGSettings       *settings;
        XKeyboardControl kbdcontrol;
        bool             click;
        int              bell_volume;
        int              bell_pitch;
        int              bell_duration;
        char            *volume_string;
        QString         volume_strings;
        int              click_volume;
        Display *dpy = QX11Info::display();
        g_debug ("Applying the bell settings");
        settings      = manager->settings;
        click         = settings->get(KEY_CLICK).toBool();
        click_volume  = settings->get(KEY_CLICK_VOLUME).toInt();

        bell_pitch    = settings->get(KEY_BELL_PITCH).toInt();
        bell_duration = settings->get(KEY_BELL_DURATION).toInt();

        volume_strings = settings->get(KEY_BELL_MODE).toChar();
        volume_string = volume_strings.toLatin1().data();
        bell_volume   = (volume_string && !strcmp (volume_string, "on")) ? 50 : 0;

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
        try {
            XChangeKeyboardControl (dpy,KBKeyClickPercent |
                                    KBBellPercent | KBBellPitch |
                                    KBBellDuration, &kbdcontrol);

            XSync (dpy, FALSE);

        } catch (int x) {

        }

}

void apply_numlock (KeyboardManager *manager)
{
        QGSettings *settings;
        bool rnumlock;
        Display *dpy = QX11Info::display();

        g_debug ("Applying the num-lock settings");
        settings = manager->settings;
        rnumlock = settings->get(KEY_NUMLOCK_REMEMBER).toBool();
        manager->old_state = settings->get(KEY_NUMLOCK_STATE).Int;
        try {
            if (rnumlock) {
                numlock_set_xkb_state ((NumLockState)manager->old_state);
            }

            XSync (dpy, FALSE);

        } catch (int x) {

        }

}

static gboolean xkb_set_keyboard_autorepeat_rate(int delay, int rate)
{
    int interval = (rate <= 0) ? 1000000 : 1000/rate;
    Display *dpy = QX11Info::display();
    if (delay <= 0)
    {
        delay = 1;
    }

    return XkbSetAutoRepeatRate(dpy, XkbUseCoreKbd, delay, interval);
}

void apply_repeat (KeyboardManager *manager)
{
        bool         repeat;
        int            rate;
        int            delay;
        Display *dpy = QX11Info::display();
        g_debug ("Applying the repeat settings");
        repeat        = manager->settings->get(KEY_REPEAT).toBool();    
        rate	      = manager->settings->get(KEY_RATE).toInt();
        delay         = manager->settings->get(KEY_DELAY).toInt();
        try {
            if (repeat) {
                gboolean rate_set = FALSE;

                XAutoRepeatOn (dpy);
                /* Use XKB in preference */
                rate_set = xkb_set_keyboard_autorepeat_rate (delay, rate);

                if (!rate_set)
                        g_warning ("Neither XKeyboard not Xfree86's keyboard extensions are available,\n"
                                   "no way to support keyboard autorepeat rate settings");
            } else {
                XAutoRepeatOff (dpy);
            }

            XSync (dpy, FALSE);

        } catch (int x) {
            CT_SYSLOG(LOG_DEBUG,"ERROR");
        }
}



void KeyboardManager::apply_settings (QString keys)
{
    /**
     * Fix by HB* system reboot but rnumlock not available;
    **/

    char *key;

    if(keys != NULL){
        key = keys.toLatin1().data();
    }else
	key=NULL;
#ifdef HAVE_X11_EXTENSIONS_XKB_H

    bool rnumlock;
    rnumlock = settings->get(KEY_NUMLOCK_REMEMBER).toBool();
    if (rnumlock == 0 || key == NULL) {
        if (have_xkb && rnumlock) {
            numlock_set_xkb_state (numlock_get_settings_state (settings));
        }
    }
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

    if (keys.compare(QString::fromLocal8Bit(KEY_CLICK)) == 0||
        keys.compare(QString::fromLocal8Bit(KEY_CLICK_VOLUME)) == 0 ||
        keys.compare(QString::fromLocal8Bit(KEY_BELL_PITCH)) == 0 ||
        keys.compare(QString::fromLocal8Bit(KEY_BELL_DURATION)) == 0 ||
        keys.compare(QString::fromLocal8Bit(KEY_BELL_MODE)) == 0) {
                g_debug ("Bell setting '%s' changed, applying bell settings", key);
                apply_bell (this);

        } else if (keys.compare(QString::fromLocal8Bit(KEY_NUMLOCK_REMEMBER)) == 0) {
                g_debug ("Remember Num-Lock state '%s' changed, applying num-lock settings", key);
                apply_numlock (this);

        } else if (keys.compare(QString::fromLocal8Bit(KEY_NUMLOCK_STATE)) == 0) {
                g_debug ("Num-Lock state '%s' changed, will apply at next startup", key);

        } else if (keys.compare(QString::fromLocal8Bit(KEY_REPEAT)) == 0 ||
                   keys.compare(QString::fromLocal8Bit(KEY_RATE)) == 0 ||
                   keys.compare(QString::fromLocal8Bit(KEY_DELAY)) == 0) {
                g_debug ("Key repeat setting '%s' changed, applying key repeat settings", key);
                apply_repeat (this);

        } else {
                g_warning ("Unhandled settings change, key '%s'", key);
        }
}

void KeyboardManager::usd_keyboard_manager_apply_settings (KeyboardManager *manager)
{
        apply_settings(NULL);
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
                        manager->settings->set(KEY_NUMLOCK_REMEMBER,numlock_state);
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


gboolean KeyboardManager::start_keyboard_idle_cb ()
{
        time->stop();

        have_xkb = 0;

        /* Essential - xkb initialization should happen before */
        mKeyXkb->usd_keyboard_xkb_init (this);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_xkb_init (this);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        /* apply current settings before we install the callback */
        usd_keyboard_manager_apply_settings (this);

        QObject::connect(settings,SIGNAL(changed(QString)),this,SLOT(apply_settings(QString)));

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_install_xkb_callback (this);

#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        return FALSE;
}
