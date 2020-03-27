#ifndef KEYBOARDXKB_H
#define KEYBOARDXKB_H

#include "keyboard-manager.h"
#include "../../config.h"

#include <string.h>
#include <time.h>

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <gio/gappinfo.h>
#include <libmatekbd/matekbd-status.h>
#include <libmatekbd/matekbd-keyboard-drawing.h>
#include <libmatekbd/matekbd-desktop-config.h>
#include <libmatekbd/matekbd-keyboard-config.h>
#include <libmatekbd/matekbd-util.h>

#define GTK_RESPONSE_PRINT 2

#define MATEKBD_DESKTOP_SCHEMA "org.mate.peripherals-keyboard-xkb.general"
#define MATEKBD_KBD_SCHEMA "org.mate.peripherals-keyboard-xkb.kbd"

#define KNOWN_FILES_KEY "known-file-list"
#define DISABLE_INDICATOR_KEY "disable-indicator"
#define DUPLICATE_LEDS_KEY "duplicate-leds"

typedef void (*PostActivationCallback) (void* userData);
class KeyboardManager;
class KeyboardXkb
{
public:
    KeyboardXkb();
    KeyboardXkb(KeyboardXkb&)=delete;
    ~KeyboardXkb();
    void usd_keyboard_xkb_init(KeyboardManager* kbd_manager);

    void usd_keyboard_xkb_shutdown(void);
    void apply_desktop_settings (void);

    static KeyboardManager * manager;

};

#endif // KEYBOARDXKB_H
