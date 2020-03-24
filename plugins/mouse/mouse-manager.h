#ifndef MOUSEMANAGER_H
#define MOUSEMANAGER_H

#include <QObject>
#include <glib.h>
#include <glib-object.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gdk/gdkkeysyms.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>
#include <gio/gio.h>
#include <glib/gspawn.h>

/* Keys with same names for both touchpad and mouse */
#define KEY_LEFT_HANDED                  "left-handed"          /*  a boolean for mouse, an enum for touchpad */
#define KEY_MOTION_ACCELERATION          "motion-acceleration"
#define KEY_MOTION_THRESHOLD             "motion-threshold"

/* Mouse settings */
#define UKUI_MOUSE_SCHEMA                "org.ukui.peripherals-mouse"
#define KEY_MOUSE_LOCATE_POINTER         "locate-pointer"
#define KEY_MIDDLE_BUTTON_EMULATION      "middle-button-enabled"

/* Touchpad settings */
#define UKUI_TOUCHPAD_SCHEMA             "org.ukui.peripherals-touchpad"
#define KEY_TOUCHPAD_DISABLE_W_TYPING    "disable-while-typing"
#define KEY_TOUCHPAD_TWO_FINGER_CLICK    "two-finger-click"
#define KEY_TOUCHPAD_THREE_FINGER_CLICK  "three-finger-click"
#define KEY_TOUCHPAD_NATURAL_SCROLL      "natural-scroll"
#define KEY_TOUCHPAD_TAP_TO_CLICK        "tap-to-click"
#define KEY_TOUCHPAD_ONE_FINGER_TAP      "tap-button-one-finger"
#define KEY_TOUCHPAD_TWO_FINGER_TAP      "tap-button-two-finger"
#define KEY_TOUCHPAD_THREE_FINGER_TAP    "tap-button-three-finger"
#define KEY_VERT_EDGE_SCROLL             "vertical-edge-scrolling"
#define KEY_HORIZ_EDGE_SCROLL            "horizontal-edge-scrolling"
#define KEY_VERT_TWO_FINGER_SCROLL       "vertical-two-finger-scrolling"
#define KEY_HORIZ_TWO_FINGER_SCROLL      "horizontal-two-finger-scrolling"
#define KEY_TOUCHPAD_ENABLED             "touchpad-enabled"

namespace spawn {
#include <glib/gspawn.h>
}

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
    bool MouseManagerStart(GError** error);
    void MouseManagerStop();

private:
    /* is include " common/usd-input-helper.c "*/
    //friend XDevice*  device_is_touchpad (XDeviceInfo *deviceinfo);
    /*----------------------------------------------------------*/

    friend gboolean usd_mouse_manager_idle_cb(MouseManager *manager);
    friend void mouse_callback (GSettings          *settings,
                                const gchar        *key,
                                MouseManager    *manager);
    friend gboolean get_touchpad_handedness (MouseManager *manager,
                                             gboolean         mouse_left_handed);
    friend void set_left_handed_all (MouseManager *manager,
                                     gboolean         mouse_left_handed,
                                     gboolean         touchpad_left_handed);
    friend void set_left_handed (MouseManager *manager,
                                 XDeviceInfo     *device_info,
                                 gboolean         mouse_left_handed,
                                 gboolean         touchpad_left_handed);
    /*friend gboolean property_exists_on_device (XDeviceInfo *device_info,
                                                  const char  *property_name);
    friend void property_set_bool (XDeviceInfo *device_info,
                                    XDevice     *device,
                                    const char  *property_name,
                                    int          property_index,
                                    gboolean     enabled);*/

    friend void set_left_handed_legacy_driver (MouseManager *manager,
                                               XDeviceInfo     *device_info,
                                               gboolean         mouse_left_handed,
                                               gboolean         touchpad_left_handed);
    /*friend gboolean touchpad_has_single_button (XDevice *device) ;
    friend void set_tap_to_click_synaptics (XDeviceInfo *device_info,
                                             gboolean     state,
                                             gboolean     left_handed,
                                             gint         one_finger_tap,
                                             gint         two_finger_tap,
                                             gint         three_finger_tap);
    friend void configure_button_layout (guchar   *buttons,
                                         gint      n_buttons,
                                         gboolean  left_handed);*/
    friend void set_motion_all (MouseManager *manager);

    friend void set_motion (MouseManager *manager, XDeviceInfo     *device_info);

    friend void set_motion_libinput (MouseManager *manager,XDeviceInfo     *device_info);

    friend void set_motion_legacy_driver (MouseManager *manager,XDeviceInfo     *device_info);

    friend void set_middle_button_all (gboolean middle_button);

    friend void set_middle_button (XDeviceInfo *device_info,
                                   gboolean     middle_button);

    friend void set_locate_pointer (MouseManager *manager, gboolean     state);

    friend void touchpad_callback (GSettings          *settings,
                                   const gchar        *key,
                                   MouseManager    *manager);

    friend void set_disable_w_typing (MouseManager *manager,
                                    gboolean         state);

    friend void set_disable_w_typing_synaptics (MouseManager *manager,
                                                gboolean         state);

    friend void set_disable_w_typing_libinput (MouseManager *manager,
                                               gboolean         state);

    friend void set_tap_to_click_all (MouseManager *manager);

    friend void set_natural_scroll_all (MouseManager *manager);

    friend void set_devicepresence_handler (MouseManager *manager);

    friend GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
                                                  GdkEvent  *event,
                                                  gpointer   data);

    friend void set_mouse_settings (MouseManager *manager);


private:
    static MouseManager * mMouseManager;
    GSettings *settings_mouse;
    GSettings *settings_touchpad;

#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
    gboolean mousetweaks_daemon_running;
#endif
    gboolean syndaemon_spawned;
    GPid syndaemon_pid;
    gboolean locate_pointer_spawned;
    GPid locate_pointer_pid;

};

#endif // MOUSEMANAGER_H
