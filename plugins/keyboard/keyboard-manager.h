#ifndef KEYBOARDMANAGER_H
#define KEYBOARDMANAGER_H

#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>

#include <QTimer>
#include <QtX11Extras/QX11Info>

#include <QGSettings/qgsettings.h>
#include <QApplication>

#include "xeventmonitor.h"
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
    void numlock_install_xkb_callback ();

public Q_SLOTS:
    void start_keyboard_idle_cb ();
    void apply_settings  (QString);
    void XkbEventsFilter(int keyCode);

private:
    friend void numlock_xkb_init (KeyboardManager *manager);
    friend void apply_bell       (KeyboardManager *manager);
    friend void apply_numlock    (KeyboardManager *manager);
    friend void apply_repeat     (KeyboardManager *manager);

    friend void numlock_install_xkb_callback (KeyboardManager *manager);

private:
    QTimer                 *time;
    static KeyboardManager *mKeyboardManager;
    static KeyboardXkb     *mKeyXkb;
    bool                    have_xkb;
    int                     xkb_event_base;
    QGSettings             *settings;
    int                     old_state;

};

#endif // KEYBOARDMANAGER_H
