#ifndef KEYBOARDMANAGER_H
#define KEYBOARDMANAGER_H

#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>

#include <QTimer>
#include <QtX11Extras/QX11Info>

#include <QGSettings/qgsettings.h>
#include <QApplication>

#include <glib.h>
#include <gdk/gdk.h>

#include "keyboard-xkb.h"

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#ifdef HAVE_X11_EXTENSIONS_XKB_H
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#endif

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
    static KeyboardManager *KeyboardManagerNew();
    bool KeyboardManagerStart();
    void KeyboardManagerStop ();
    void usd_keyboard_manager_apply_settings(KeyboardManager *manager);

public Q_SLOTS:
    void start_keyboard_idle_cb ();
    void apply_settings  (QString);

private:
    friend void numlock_xkb_init (KeyboardManager *manager);
    friend void apply_bell       (KeyboardManager *manager);
    friend void apply_numlock    (KeyboardManager *manager);
    friend void apply_repeat     (KeyboardManager *manager);

    friend void numlock_install_xkb_callback (KeyboardManager *manager);
    friend GdkFilterReturn xkb_events_filter (GdkXEvent *xev_,
                                              GdkEvent  *gdkev_,
                                              gpointer   user_data);

/*
protected:
    bool eventFilter(QObject *watched, QEvent *event);
*/
private:
    QTimer                 *time;
    static KeyboardManager *mKeyboardManager;
    static KeyboardXkb     *mKeyXkb;
    gboolean                have_xkb;
    int                     xkb_event_base;
    QGSettings             *settings;
    int                     old_state;

};

#endif // KEYBOARDMANAGER_H
