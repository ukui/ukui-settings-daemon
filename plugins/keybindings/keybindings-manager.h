#ifndef KEYBINDINGSMANAGER_H
#define KEYBINDINGSMANAGER_H

#include <QObject>
#include <QTimer>
#include <QtX11Extras/QX11Info>
#include <QGSettings>
#include <QApplication>
#include <QKeyEvent>
#include <QMessageBox>
#include <QScreen>

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
#include <X11/keysym.h>
#include <gio/gio.h>
#include <dconf.h>
extern "C"{
#include "ukui-keygrab.h"
#include "eggaccelerators.h"
}

typedef struct {
        char *binding_str;
        char *action;
        char *settings_path;
        Key   key;
        Key   previous_key;
} Binding;

class KeybindingsManager : public QObject
{
    Q_OBJECT
private:
    KeybindingsManager();
    KeybindingsManager(KeybindingsManager&)=delete;

public:
    ~KeybindingsManager();
    static KeybindingsManager *KeybindingsManagerNew();
    bool KeybindingsManagerStart();
    void KeybindingsManagerStop();

public:
    static void bindings_callback (DConfClient  *client,
                                   gchar        *prefix,
                                   GStrv        changes,
                                   gchar        *tag);
    static bool key_already_used (Binding  *binding);
    static void binding_register_keys ();
    static void binding_unregister_keys ();
    static void bindings_clear();
    static void bindings_get_entries();
    static bool bindings_get_entry (const char *settings_path);

    friend GdkFilterReturn keybindings_filter (GdkXEvent           *gdk_xevent,
                                               GdkEvent            *event,
                                               KeybindingsManager  *manager);

private:
    static KeybindingsManager *mKeybinding;
    DConfClient *client;
    static GSList   *binding_list;
    static GSList   *screens;

};

#endif // KEYBINDINGSMANAGER_H
