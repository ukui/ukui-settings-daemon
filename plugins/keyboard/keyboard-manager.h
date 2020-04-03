#ifndef KEYBOARDMANAGER_H
#define KEYBOARDMANAGER_H

#include <QObject>
#include <QTimer>
#include <QtX11Extras/QX11Info>
#include <QGSettings>
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
#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include "keyboard-xkb.h"

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#ifdef HAVE_X11_EXTENSIONS_XKB_H
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#endif

#define USD_KEYBOARD_SCHEMA "org.ukui.peripherals-keyboard"

#define KEY_REPEAT         "repeat"
#define KEY_CLICK          "click"
#define KEY_RATE           "rate"
#define KEY_DELAY          "delay"
#define KEY_CLICK_VOLUME   "click-volume"

#define KEY_BELL_PITCH     "bell-pitch"
#define KEY_BELL_DURATION  "bell-duration"
#define KEY_BELL_MODE      "bell-mode"

#define KEY_NUMLOCK_STATE    "numlock-state"
#define KEY_NUMLOCK_REMEMBER "remember-numlock-state"

typedef enum {
        NUMLOCK_STATE_OFF = 0,
        NUMLOCK_STATE_ON = 1,
        NUMLOCK_STATE_UNKNOWN = 2
} NumLockState;

class KeyboardXkb;
class KeyboardManager : public QObject
{
    Q_OBJECT
private:
    KeyboardManager()=delete;
    KeyboardManager(KeyboardManager&)=delete;
    KeyboardManager&operator=(const KeyboardManager&)=delete;
    KeyboardManager(QObject *parent = nullptr);
public:
    ~KeyboardManager();
    static KeyboardManager * KeyboardManagerNew();
    bool KeyboardManagerStart();
    void KeyboardManagerStop();
    void usd_keyboard_manager_apply_settings (KeyboardManager *manager);

public Q_SLOTS:
    gboolean start_keyboard_idle_cb ();
    void apply_settings(QString);

private:
    friend void numlock_xkb_init (KeyboardManager *manager);

    friend void apply_bell (KeyboardManager *manager);

    friend void apply_numlock (KeyboardManager *manager);

    friend void apply_repeat (KeyboardManager *manager);

    friend void numlock_install_xkb_callback (KeyboardManager *manager);

    friend GdkFilterReturn xkb_events_filter (GdkXEvent *xev_,
                                   GdkEvent  *gdkev_,
                                   gpointer   user_data);

private:
    QTimer * time;
    static KeyboardManager* mKeyboardManager;
    static KeyboardXkb * mKeyXkb;
    gboolean    have_xkb;
    gint        xkb_event_base;
    QGSettings  *settings;
    int 	    old_state;

};

#endif // KEYBOARDMANAGER_H
