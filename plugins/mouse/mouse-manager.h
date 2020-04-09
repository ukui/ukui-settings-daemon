#ifndef MOUSEMANAGER_H
#define MOUSEMANAGER_H

#include <QObject>
#include <QTimer>
#include <QtX11Extras/QX11Info>
#include <QGSettings>
#include <QApplication>

#include <glib.h>
#include <errno.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

class MouseManager : public QObject
{
    Q_OBJECT

private:
    MouseManager()=delete;
    MouseManager(MouseManager&)=delete;
    MouseManager&operator=(const MouseManager&)=delete;
    MouseManager(QObject *parent = nullptr);
public:
    ~MouseManager();
    static MouseManager * MouseManagerNew();
    bool MouseManagerStart();
    void MouseManagerStop();

public Q_SLOTS:
    void usd_mouse_manager_idle_cb();
    void mouse_callback(QString);
    void touchpad_callback(QString);

private:
    friend bool get_touchpad_handedness (MouseManager *manager,
                                         bool mouse_left_handed);
    friend void set_left_handed_all     (MouseManager *manager,
                                         bool mouse_left_handed,
                                         bool touchpad_left_handed);
    friend void set_left_handed         (MouseManager *manager,
                                         XDeviceInfo     *device_info,
                                         bool         mouse_left_handed,
                                         bool         touchpad_left_handed);

    friend void set_left_handed_legacy_driver (MouseManager *manager,
                                               XDeviceInfo     *device_info,
                                               bool         mouse_left_handed,
                                               bool         touchpad_left_handed);

    friend void set_motion_all          (MouseManager *manager);

    friend void set_motion              (MouseManager *manager,
                                         XDeviceInfo  *device_info);

    friend void set_motion_libinput     (MouseManager *manager,
                                         XDeviceInfo  *device_info);

    friend void set_motion_legacy_driver(MouseManager *manager,
                                         XDeviceInfo     *device_info);

    friend void set_middle_button_all   (bool middle_button);

    friend void set_middle_button       (XDeviceInfo *device_info,
                                        bool     middle_button);

    friend void set_locate_pointer      (MouseManager *manager, bool     state);

    friend void set_disable_w_typing    (MouseManager *manager,
                                        bool         state);

    friend void set_disable_w_typing_synaptics (MouseManager *manager,
                                                bool         state);

    friend void set_disable_w_typing_libinput (MouseManager *manager,
                                               bool         state);

    friend void set_tap_to_click_all    (MouseManager *manager);

    friend void set_natural_scroll_all  (MouseManager *manager);

    friend void set_devicepresence_handler (MouseManager *manager);

    friend GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
                                                  GdkEvent  *event,
                                                  gpointer   data);

    friend void set_mouse_settings (MouseManager *manager);


private:
    QTimer * time;
    QGSettings *settings_mouse;
    QGSettings *settings_touchpad;
    static MouseManager *mMouseManager;

#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
    gboolean mousetweaks_daemon_running;
#endif
    gboolean syndaemon_spawned;
    GPid syndaemon_pid;
    gboolean locate_pointer_spawned;
    GPid locate_pointer_pid;

};

#endif // MOUSEMANAGER_H
