#ifndef KEYBOARDXKB_H
#define KEYBOARDXKB_H

#include "keyboard-manager.h"
#include "config.h"
#include <string.h>
#include <time.h>

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gappinfo.h>
extern "C"{
#include <matekbd-status.h>
#include <matekbd-keyboard-drawing.h>
#include <matekbd-desktop-config.h>
#include <matekbd-keyboard-config.h>
#include <matekbd-util.h>
}


#define GTK_RESPONSE_PRINT 2

#define MATEKBD_DESKTOP_SCHEMA "org.mate.peripherals-keyboard-xkb.general"
#define MATEKBD_KBD_SCHEMA "org.mate.peripherals-keyboard-xkb.kbd"

#define KNOWN_FILES_KEY "known-file-list"
#define DISABLE_INDICATOR_KEY "disable-indicator"
#define DUPLICATE_LEDS_KEY "duplicate-leds"

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
    friend void apply_desktop_settings_mate_cb(GSettings *settings, gchar *key, gpointer   user_data);


};

#endif // KEYBOARDXKB_H
