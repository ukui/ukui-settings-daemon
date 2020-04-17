#ifndef KEYBOARDXKB_H
#define KEYBOARDXKB_H

#include "keyboard-manager.h"
#include "config.h"
#include <string.h>
#include <time.h>

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gio/gio.h>

extern "C"{
#include <matekbd-status.h>
#include <matekbd-keyboard-drawing.h>
#include <matekbd-desktop-config.h>
#include <matekbd-keyboard-config.h>
#include <matekbd-util.h>
}

typedef void (*PostActivationCallback) (void* userData);
class KeyboardManager;

class KeyboardXkb : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    static void apply_desktop_settings_cb (QString);
    static void apply_xkb_settings_cb(QString);

public:
    KeyboardXkb();
    KeyboardXkb(KeyboardXkb&)=delete;
    ~KeyboardXkb();
    static KeyboardManager * manager;

public:
    void usd_keyboard_xkb_init(KeyboardManager* kbd_manager);
    void usd_keyboard_xkb_shutdown(void);
    static void apply_desktop_settings (void);
    static void usd_keyboard_new_device (XklEngine * engine);
    static void apply_desktop_settings_mate_cb(GSettings *settings, gchar *key);
    static void apply_xkb_settings_mate_cb (GSettings *settings, gchar *key);
    static void apply_xkb_settings (void);
    friend GdkFilterReturn usd_keyboard_xkb_evt_filter (GdkXEvent * xev, GdkEvent * event,gpointer data);
    void usd_keyboard_xkb_analyze_sysconfig (void);
    static bool try_activating_xkb_config_if_new (MatekbdKeyboardConfig *current_sys_kbd_config);
    static bool filter_xkb_config (void);

public:
    QGSettings* settings_desktop;
    QGSettings* settings_kbd;
};

#endif // KEYBOARDXKB_H
