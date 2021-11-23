/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "mouse-manager.h"
#include "clib-syslog.h"
#include "usd_base_class.h"

/* Keys with same names for both touchpad and mouse */
#define KEY_LEFT_HANDED                  "left-handed"          /*  a boolean for mouse, an enum for touchpad */
#define KEY_MOTION_ACCELERATION          "motion-acceleration"
#define KEY_MOTION_THRESHOLD             "motion-threshold"

/* Mouse settings */
#define UKUI_MOUSE_SCHEMA                "org.ukui.peripherals-mouse"
#define KEY_MOUSE_LOCATE_POINTER         "locate-pointer"
#define KEY_MIDDLE_BUTTON_EMULATION      "middle-button-enabled"
#define KEY_MOUSE_WHEEL_SPEED            "wheel-speed"
#define KEY_MOUSE_ACCEL                  "mouse-accel"

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

#define KEY_TOUCHPAD_DISBLE_O_E_MOUSE    "disable-on-external-mouse"        //插入鼠标，禁用触摸板  true/false
#define KEY_TOUCHPAD_DOUBLE_CLICK_DRAG   "double-click-drag"                //点击两次拖动		true/false
#define KEY_TOUCHPAD_BOTTOM_R_C_CLICK_M  "bottom-right-corner-click-menu"   //右下角点击菜单	true/false
#define KEY_TOUCHPAD_MOUSE_SENSITVITY    "mouse-sensitivity"                //鼠标敏感度	1-4  四个档位  低中高最高


extern "C"{
#include <X11/extensions/XInput2.h>
}

typedef enum {
        TOUCHPAD_HANDEDNESS_RIGHT,
        TOUCHPAD_HANDEDNESS_LEFT,
        TOUCHPAD_HANDEDNESS_MOUSE
} TouchpadHandedness;


GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
                                       GdkEvent  *event,
                                       gpointer   data);

bool supports_xinput_devices (void);
bool  touchpad_is_present     (void);

MouseManager * MouseManager::mMouseManager =nullptr;

MouseManager::MouseManager(QObject *parent) : QObject (parent)
{
    gdk_init(NULL,NULL);
    syndaemon_spawned = false;
    syndaemon_pid   = 0;
    locate_pointer_spawned = false;
    locate_pointer_pid  = 0;
    imwheelSpawned  = false;
    settings_mouse  = new QGSettings(UKUI_MOUSE_SCHEMA);
    settings_touchpad = new QGSettings(UKUI_TOUCHPAD_SCHEMA);
}
MouseManager::~MouseManager()
{
    delete settings_mouse;
    delete settings_touchpad;
    if(time)
        delete time;
}

MouseManager * MouseManager::MouseManagerNew()
{
    if(nullptr == mMouseManager)
        mMouseManager = new MouseManager(nullptr);

    return mMouseManager;
}

bool MouseManager::MouseManagerStart()
{
    USD_LOG(LOG_DEBUG,"-- Mouse Start Manager --");

    if (!supports_xinput_devices()){
        qWarning("XInput is not supported, not applying any settings");
        return TRUE;
    }
    time = new QTimer(this);
    connect(time, &QTimer::timeout, this, &MouseManager::MouseManagerIdleCb);
    time->start();
    return true;
}

void MouseManager::MouseManagerStop()
{

    USD_LOG(LOG_DEBUG,"-- Stopping Mouse Manager --");

    SetLocatePointer(FALSE);

    gdk_window_remove_filter (NULL, devicepresence_filter, this);
}

/*  transplant usd-input-helper.h  */
/* Checks whether the XInput device is supported
 * 检测是否支持xinput设备
 */
bool
supports_xinput_devices (void)
{
    int op_code, event, error;

    return XQueryExtension (QX11Info::display(),
                            "XInputExtension",
                            &op_code,
                            &event,
                            &error);
}

static bool
device_has_property (XDevice    *device,
                     const char *property_name)
{
    Atom realtype, prop;
    int realformat;
    unsigned long nitems, bytes_after;
    unsigned char *data;

    prop = XInternAtom (QX11Info::display(), property_name, True);
    if (!prop)
            return FALSE;

    try {
        if ((XGetDeviceProperty (QX11Info::display(), device, prop, 0, 1, False,
                                XA_INTEGER, &realtype, &realformat, &nitems,
                                &bytes_after, &data) == Success) && (realtype != None)) {
                XFree (data);
                return TRUE;
        }

    } catch (int x) {

    }
    return FALSE;
}

XDevice* device_is_touchpad (XDeviceInfo *deviceinfo)
{
    XDevice *device;

    if (deviceinfo->type != XInternAtom (QX11Info::display(), XI_TOUCHPAD, True))
            return NULL;

    try {
        device = XOpenDevice (QX11Info::display(), deviceinfo->id);
        if (device == NULL)
            throw 1;

        if (device_has_property (device, "libinput Tapping Enabled") ||
            device_has_property (device, "Synaptics Off")) {
                return device;
        }
        XCloseDevice (QX11Info::display(), device);

    } catch (int x) {
        return NULL;
    }
    return NULL;
}

bool touchpad_is_present (void)
{
    XDeviceInfo *device_info;
    int     n_devices;
    int    i;
    bool    retval;

    if (supports_xinput_devices () == false)
            return true;

    retval = false;

    device_info = XListInputDevices (QX11Info::display(), &n_devices);
    if (device_info == NULL)
            return false;

    for (i = 0; i < n_devices; i++) {
            XDevice *device;

            device = device_is_touchpad (&device_info[i]);
            if (device != NULL) {
                    retval = true;
                    break;
            }
    }
    if (device_info != NULL)
            XFreeDeviceList (device_info);

    return retval;
}



bool MouseManager::GetTouchpadHandedness (bool  mouse_left_handed)
{
    int a = settings_touchpad->getEnum(KEY_LEFT_HANDED);

    switch (a) {
    case TOUCHPAD_HANDEDNESS_RIGHT:
            return false;
    case TOUCHPAD_HANDEDNESS_LEFT:
            return true;
    case TOUCHPAD_HANDEDNESS_MOUSE:
            return mouse_left_handed;
    default:
            g_assert_not_reached ();
    }
}
Atom property_from_name (const char *property_name)
{
    return XInternAtom (QX11Info::display(), property_name, True);
}

bool property_exists_on_device (XDeviceInfo *device_info, const char  *property_name)
{
    XDevice *device;
    int rc;
    Atom type, prop;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data;
   // Display *display = gdk_x11_get_default_xdisplay ();//QX11Info::display();
    Display * display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    prop = property_from_name (property_name);

    if (!prop)
            return FALSE;
    try {
        device = XOpenDevice (display, device_info->id);

        if (device == NULL) {
            USD_LOG(LOG_DEBUG, "%s find %s had a error:can't open id(%d) device in XOpenDevice,type：%d",device_info->name, property_name,device_info->id,device_info->type);
            throw 1;
        }
//        USD_LOG(LOG_DEBUG,"prop [%s] had find in %d type:%d", property_name, device_info->id, device_info->type);
        rc = XGetDeviceProperty (display,
                                 device, prop, 0, 1, False, XA_INTEGER, &type, &format,
                                 &nitems, &bytes_after, &data);
        if (rc == Success)
                XFree (data);

        XCloseDevice (display, device);
    } catch (int str) {
        USD_LOG(LOG_DEBUG,"MOUSE: WRING ID: %d",str);
        return FALSE;
    }

    return rc == Success;
}

void property_set_bool (XDeviceInfo *device_info,
                        XDevice     *device,
                        const char  *property_name,
                        int          property_index,
                        bool         enabled)
{
    int rc;
    unsigned long nitems, bytes_after;
    unsigned char *data;
    int act_format;
    Atom act_type, property;
    Display *display = gdk_x11_get_default_xdisplay ();//QX11Info::display();
    property = property_from_name (property_name);
    if (!property)
            return;

    try {
        gdk_x11_display_error_trap_push (gdk_display_get_default());
        rc = XGetDeviceProperty (display, device,
                                 property, 0, 1, False,
                                 XA_INTEGER, &act_type, &act_format, &nitems,
                                 &bytes_after, &data);
        if (rc == Success && act_type == XA_INTEGER && act_format == 8 && nitems >(unsigned long)property_index) {
                data[property_index] = enabled ? 1 : 0;

                XChangeDeviceProperty (display, device,
                                       property, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }
        if (rc == Success)
                XFree (data);
        if(gdk_x11_display_error_trap_pop (gdk_display_get_default()))
            qWarning ("Error while setting %s on \"%s\"", property_name, device_info->name);

    } catch (int x) {
        USD_LOG(LOG_DEBUG,"MOUSE:Error while setting %s on \"%s\"", property_name, device_info->name)

    }
}

void set_left_handed_libinput (XDeviceInfo *device_info,
                          bool     mouse_left_handed,
                          bool     touchpad_left_handed)
{
    XDevice *device;
    bool want_lefthanded;
    Display *display = QX11Info::display();
    device = device_is_touchpad (device_info);

    try {
        if (device == NULL) {
                device = XOpenDevice (display, device_info->id);
                if (device == NULL)
                    throw 1;

                want_lefthanded = mouse_left_handed;
        } else {
                /* touchpad device is already open after
                 * return from device_is_touchpad function
                 */
                want_lefthanded = touchpad_left_handed;
        }
        property_set_bool (device_info, device, "libinput Left Handed Enabled", 0, want_lefthanded);
        XCloseDevice (display, device);
    } catch (int x) {
        return;
    }
}

gboolean xinput_device_has_buttons (XDeviceInfo *device_info)
{
    int i;
    XAnyClassInfo *class_info;

    class_info = device_info->inputclassinfo;
    for (i = 0; i < device_info->num_classes; i++) {
            if (class_info->c_class == ButtonClass) {
                    XButtonInfo *button_info;

                    button_info = (XButtonInfo *) class_info;
                    if (button_info->num_buttons > 0)
                            return TRUE;
            }

            class_info = (XAnyClassInfo *) (((guchar *) class_info) +
                                            class_info->length);
    }
    return FALSE;
}

bool touchpad_has_single_button (XDevice *device)
{
        Atom type, prop;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        bool is_single_button = FALSE;
        int rc;

        prop = property_from_name ("Synaptics Capabilities");
        if (!prop)
                return false;

        try {
            rc = XGetDeviceProperty (QX11Info::display(), device, prop, 0, 1, False,
                                     XA_INTEGER, &type, &format, &nitems,
                                     &bytes_after, &data);
            if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3)
                    is_single_button = (data[0] == 1 && data[1] == 0 && data[2] == 0);

            if (rc == Success)
                    XFree (data);
        } catch (int x) {

        }

        return is_single_button;
}

void set_tap_to_click_synaptics (XDeviceInfo *device_info,
                                 bool         state,
                                 bool         left_handed,
                                 int         one_finger_tap,
                                 int         two_finger_tap,
                                 int         three_finger_tap)
{
    XDevice *device;
    int format, rc;
    unsigned long nitems, bytes_after;
    unsigned char* data;
    Atom prop, type;
    Display *display = gdk_x11_get_default_xdisplay (); //QX11Info::display();
    prop = property_from_name ("Synaptics Tap Action");

    if (!prop)
            return;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
            return;
    }

    try {
        rc = XGetDeviceProperty (display, device, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (one_finger_tap > 3 || one_finger_tap < 1)
                one_finger_tap = 1;
        if (two_finger_tap > 3 || two_finger_tap < 1)
                two_finger_tap = 3;
        if (three_finger_tap > 3 || three_finger_tap < 0)
                three_finger_tap = 0;

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 7)
        {
                /* Set RLM mapping for 1/2/3 fingers*/
                data[4] = (state) ? ((left_handed) ? (4-one_finger_tap) : one_finger_tap) : 0;
                data[5] = (state) ? ((left_handed) ? (4-two_finger_tap) : two_finger_tap) : 0;
                data[6] = (state) ? three_finger_tap : 0;
                XChangeDeviceProperty (display, device, prop, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (display, device);
    } catch (int x) {
        qWarning("Error in setting tap to click on \"%s\"", device_info->name);
    }
}

void configure_button_layout (guchar   *buttons,
                              int      n_buttons,
                              bool      left_handed)
{
    const int left_button = 1;
    int right_button;
    int i;

    /* if the button is higher than 2 (3rd button) then it's
     * probably one direction of a scroll wheel or something else
     * uninteresting
     */
    right_button = MIN (n_buttons, 3);

    /* If we change things we need to make sure we only swap buttons.
     * If we end up with multiple physical buttons assigned to the same
     * logical button the server will complain. This code assumes physical
     * button 0 is the physical left mouse button, and that the physical
     * button other than 0 currently assigned left_button or right_button
     * is the physical right mouse button.
     */
    /* check if the current mapping satisfies the above assumptions */
    if (buttons[left_button - 1] != left_button &&
        buttons[left_button - 1] != right_button)
            /* The current mapping is weird. Swapping buttons is probably not a
             * good idea.
             */
            return;

    /* check if we are left_handed and currently not swapped */
    if (left_handed && buttons[left_button - 1] == left_button) {
            /* find the right button */
            for (i = 0; i < n_buttons; i++) {
                    if (buttons[i] == right_button) {
                            buttons[i] = left_button;
                            break;
                    }
            }
            /* swap the buttons */
            buttons[left_button - 1] = right_button;
    }
    /* check if we are not left_handed but are swapped */
    else if (!left_handed && buttons[left_button - 1] == right_button) {
            /* find the right button */
            for (i = 0; i < n_buttons; i++) {
                    if (buttons[i] == left_button) {
                            buttons[i] = right_button;
                            break;
                    }
            }
            /* swap the buttons */
            buttons[left_button - 1] = left_button;
    }
}

void MouseManager::SetLeftHandedLegacyDriver (XDeviceInfo     *device_info,
                                             bool         mouse_left_handed,
                                             bool         touchpad_left_handed)
{
    XDevice *device;
    unsigned char *buttons;
    unsigned long  buttons_capacity = 16;
    int     n_buttons;
    bool    left_handed;
    Display *display = QX11Info::display();
    if ((device_info->use == IsXPointer) ||
        (device_info->use == IsXKeyboard) ||
        (g_strcmp0 ("Virtual core XTEST pointer", device_info->name) == 0) ||
        (!xinput_device_has_buttons (device_info)))
            return;

    /* If the device is a touchpad, swap tap buttons
     * around too, otherwise a tap would be a right-click */
    device = device_is_touchpad (device_info);
    if (device != NULL) {
            bool tap = settings_touchpad->get(KEY_TOUCHPAD_TAP_TO_CLICK).toBool();
            bool single_button = touchpad_has_single_button (device);

            left_handed = touchpad_left_handed;

            if (tap && !single_button) {
                    int one_finger_tap = settings_touchpad->get(KEY_TOUCHPAD_ONE_FINGER_TAP).toInt();
                    int two_finger_tap = settings_touchpad->get(KEY_TOUCHPAD_TWO_FINGER_TAP).toInt();
                    int three_finger_tap = settings_touchpad->get(KEY_TOUCHPAD_THREE_FINGER_TAP).toInt();
                    set_tap_to_click_synaptics (device_info, tap, left_handed, one_finger_tap, two_finger_tap, three_finger_tap);
            }

            XCloseDevice (display, device);
            if (single_button)
                    return;
    } else {
            left_handed = mouse_left_handed;
    }

    try {
        device = XOpenDevice (display, device_info->id);
        if (device == NULL)
            throw 1;

        buttons = g_new (guchar, buttons_capacity);

        n_buttons = XGetDeviceButtonMapping (display, device,
                                             buttons,
                                             buttons_capacity);

        while (n_buttons > (int)buttons_capacity) {
                buttons_capacity = n_buttons;
                buttons = (guchar *) g_realloc (buttons,
                                                buttons_capacity * sizeof (guchar));

                n_buttons = XGetDeviceButtonMapping (display, device,
                                                     buttons,
                                                     buttons_capacity);
        }

        configure_button_layout (buttons, n_buttons, left_handed);

        XSetDeviceButtonMapping (display, device, buttons, n_buttons);
        XCloseDevice (display, device);

        g_free (buttons);
    } catch (int x) {
        USD_LOG(LOG_DEBUG,"MOUSE :error id %d",x);
        return;
    }
}

void MouseManager::SetLeftHanded (XDeviceInfo  *device_info,
                      bool         mouse_left_handed,
                      bool         touchpad_left_handed)
{
    if (property_exists_on_device (device_info, "libinput Left Handed Enabled")){
        set_left_handed_libinput (device_info, mouse_left_handed, touchpad_left_handed);

    } else {
        SetLeftHandedLegacyDriver (device_info, mouse_left_handed, touchpad_left_handed);
    }
}

void MouseManager::SetLeftHandedAll (bool mouse_left_handed,
                                     bool touchpad_left_handed)
{
    XDeviceInfo *device_info = nullptr;
    int n_devices;
    int i;
    Display * dpy = QX11Info::display();
    device_info = XListInputDevices (dpy, &n_devices);
    if(!device_info){
        qWarning("SetLeftHandedAll: device_info is null");
        return;
    }

    for (i = 0; i < n_devices; i++) {
        SetLeftHanded (&device_info[i], mouse_left_handed, touchpad_left_handed);
    }
    if (device_info != NULL)
        XFreeDeviceList (device_info);
}

void MouseManager::SetMotionLibinput (XDeviceInfo *device_info)
{
    XDevice *device;
    Atom prop;
    Atom type;
    Atom float_type;
    int format, rc;
    unsigned long nitems, bytes_after;
    QGSettings *settings;

    Display * dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());// gdk_x11_get_default_xdisplay ();//QX11Info::display();

    union {
        unsigned char *c;
        long *l;
    } data;
    float accel;
    float motion_acceleration;

    float_type = property_from_name ("FLOAT");
    if (!float_type)
        return;

    prop = property_from_name ("libinput Accel Speed");
    if (!prop) {
        return;
    }
    try {
        device = device_is_touchpad (device_info);
        if (device != NULL) {
            qDebug()<<"device != NULL  settings = settings_touchpad";
            settings = settings_touchpad;
        } else {
            device = XOpenDevice (dpy, device_info->id);
            if (device == NULL)
                throw 1;
            settings = settings_mouse;
        }
        /* Calculate acceleration */
        motion_acceleration = settings->get(KEY_MOTION_ACCELERATION).toDouble();

        /* panel gives us a range of 1.0-10.0, map to libinput's [-1, 1]
         *
         * oldrange = (oldmax - oldmin)
         * newrange = (newmax - newmin)
         *
         * mapped = (value - oldmin) * newrange / oldrange + oldmin
         */

        if (motion_acceleration == -1.0) /* unset */
                accel = 0.0;
        else
                accel = (motion_acceleration - 1.0) * 2.0 / 9.0 - 1;

        rc = XGetDeviceProperty (dpy,device, prop, 0, 1, False, float_type, &type,
                                 &format, &nitems, &bytes_after, &data.c);
        
        if (rc == Success && type == float_type && format == 32 && nitems >= 1) {
                *(float *) data.l = accel;
                XChangeDeviceProperty (dpy, device, prop, float_type, 32,
                                       PropModeReplace, data.c, nitems);
        }
        if (rc == Success) {
                XFree (data.c);
        }
        XCloseDevice (dpy, device);

    } catch (int x) {
        qWarning("%s Error while setting accel speed on \"%s\"", device_info->name);
        return;
    }
}

void MouseManager::SetMotionLegacyDriver (XDeviceInfo     *device_info)
{
    XDevice *device;
    XPtrFeedbackControl feedback;
    XFeedbackState *states, *state;
    int num_feedbacks, i;
    QGSettings *settings;
    double motion_acceleration;
    int motion_threshold;
    int numerator, denominator;

    Display * dpy = gdk_x11_get_default_xdisplay ();//QX11Info::display();

    device = device_is_touchpad (device_info);
    if (device != NULL) {
            settings = settings_touchpad;
    } else {
        try {
            device = XOpenDevice (dpy, device_info->id);
            if (device == NULL)
                throw 1;
        } catch (int x) {
            USD_LOG(LOG_DEBUG,"%s: error id %d","MOUSE", x);
            return;
        }
            settings = settings_mouse;
    }

    /* Calculate acceleration */
    motion_acceleration = settings->get(KEY_MOTION_ACCELERATION).toDouble();

    if (motion_acceleration >= 1.0) {
            /* we want to get the acceleration, with a resolution of 0.5
             */
            if ((motion_acceleration - floor (motion_acceleration)) < 0.25) {
                    numerator = floor (motion_acceleration);
                    denominator = 1;
            } else if ((motion_acceleration - floor (motion_acceleration)) < 0.5) {
                    numerator = ceil (2.0 * motion_acceleration);
                    denominator = 2;
            } else if ((motion_acceleration - floor (motion_acceleration)) < 0.75) {
                    numerator = floor (2.0 *motion_acceleration);
                    denominator = 2;
            } else {
                    numerator = ceil (motion_acceleration);
                    denominator = 1;
            }
    } else if (motion_acceleration < 1.0 && motion_acceleration > 0) {
            /* This we do to 1/10ths */
            numerator = floor (motion_acceleration * 10) + 1;
            denominator= 10;
    } else {
            numerator = -1;
            denominator = -1;
    }

    /* And threshold */
    motion_threshold = settings->get(KEY_MOTION_THRESHOLD).toInt();
//    qDebug()<<__func__<<" motion_threshold = "<<motion_threshold;
    USD_LOG(LOG_DEBUG,"motion_threshold:%d", motion_threshold);
    /* Get the list of feedbacks for the device */
    states = XGetFeedbackControl (dpy, device, &num_feedbacks);
    if (states == NULL) {
            XCloseDevice (dpy, device);
            return;
    }

    state = (XFeedbackState *) states;
    for (i = 0; i < num_feedbacks; i++) {
        if (state->c_class == PtrFeedbackClass) {
            /* And tell the device */
            feedback.c_class      = PtrFeedbackClass;
            feedback.length     = sizeof (XPtrFeedbackControl);
            feedback.id         = state->id;
            feedback.threshold  = motion_threshold;
            feedback.accelNum   = numerator;
            feedback.accelDenom = denominator;

            USD_LOG(LOG_DEBUG,"Setting accel %d/%d, threshold %d for device '%s'",
                     numerator, denominator, motion_threshold, device_info->name);

            XChangeFeedbackControl (dpy,
                                    device,
                                    DvAccelNum | DvAccelDenom | DvThreshold,
                                    (XFeedbackControl *) &feedback);
            break;
        }
        state = (XFeedbackState *) ((char *) state + state->length);
    }
    XFreeFeedbackList (states);
    XCloseDevice (dpy, device);
}

void MouseManager::SetTouchpadMotionAccel(XDeviceInfo *device_info)
{
    XDevice *device;
    Atom prop;
    Atom type;
    Atom float_type;
    int format, rc;
    unsigned long nitems, bytes_after;


//    Display * dpy = gdk_x11_get_default_xdisplay ();//QX11Info::display();
    Display * dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());//Display * dpy = QX11Info::display();
    union {
        unsigned char *c;
        long *l;
    } data;
    float accel;
    float motion_acceleration;

    float_type = property_from_name ("FLOAT");
    if (!float_type)
        return;

    prop = property_from_name ("Device Accel Constant Deceleration");
    if (!prop) {
        return;
    }
    try {
        device = device_is_touchpad (device_info);
        if (device == NULL)
            return;
        /* Calculate acceleration */
        motion_acceleration = settings_touchpad->get(KEY_MOTION_ACCELERATION).toDouble();
        if (motion_acceleration == -1.0) /* unset */
                accel = 0.0;
        else
                accel = motion_acceleration;

        rc = XGetDeviceProperty (dpy,device, prop, 0, 1, False, float_type, &type,
                                 &format, &nitems, &bytes_after, &data.c);
        qDebug()<<"format = "<<format<<"nitems = "<<nitems;
        if (rc == Success && type == float_type && format == 32 && nitems >= 1) {
                *(float *) data.l = accel;
                XChangeDeviceProperty (dpy, device, prop, float_type, 32,
                                       PropModeReplace, data.c, nitems);
        }
        if (rc == Success) {
                XFree (data.c);
        }
        XCloseDevice (dpy, device);

    } catch (int x) {
        USD_LOG(LOG_ERR,"catch a bug...");
//        qWarning("%s Error while setting accel speed on \"%s\"", device_info->name);
        return;
    }
}
void MouseManager::SetMouseAccel(XDeviceInfo *device_info)
{
    XDevice *device;
    Atom prop;
    Atom type;
    int format, rc;
    unsigned long nitems, bytes_after;

    //Display * dpy = QX11Info::display();
    Display * dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    unsigned char *data;
    bool MouseAccel;

    prop = property_from_name ("libinput Accel Profile Enabled");
    if (!prop) {
        return;
    }

    try {
        device = XOpenDevice (dpy, device_info->id);
        if (device == NULL)
            throw 1;
        rc = XGetDeviceProperty (dpy,device, prop, 0, 2, False, XA_INTEGER, &type,
                                 &format, &nitems, &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 1) {
            MouseAccel = settings_mouse->get(KEY_MOUSE_ACCEL).toBool();
            if(MouseAccel){
                data[0] = 1;
                data[1] = 0;
            }else{
                data[0] = 0;
                data[1] = 1;
            }
            XChangeDeviceProperty (dpy, device, prop, XA_INTEGER, 8,
                                   PropModeReplace, data, nitems);
        }
        if (rc == Success) {
                XFree (data);
        }
        XCloseDevice (dpy, device);

    } catch (int x) {
        USD_LOG(LOG_ERR,"catch a bug...");
//        qWarning("%s Error while setting accel speed on \"%s\"", device_info->name);
        return;
    }
}

void MouseManager::SetMotion (XDeviceInfo *device_info)
{
    if (property_exists_on_device (device_info, "libinput Accel Speed")) {

        SetMotionLibinput (device_info);
    }
    else {
        SetMotionLegacyDriver (device_info);
    }

    if(property_exists_on_device (device_info, "Device Accel Constant Deceleration")) {

        SetTouchpadMotionAccel(device_info);
    }

    if(property_exists_on_device (device_info, "libinput Accel Profile Enabled")) {

        SetMouseAccel(device_info);
    }
}

void MouseManager::SetMotionAll()
{
    XDeviceInfo *device_info = nullptr;
    int n_devices;
    int i;

    device_info = XListInputDevices (gdk_x11_get_default_xdisplay (), &n_devices);
    if(!device_info){
        qWarning("SetMotionAll: device_info is null");
        return;
    }
    for (i = 0; i < n_devices; i++) {
        SetMotion (&device_info[i]);
    }

    if (device_info != NULL)
        XFreeDeviceList (device_info);
}

void set_middle_button_evdev (XDeviceInfo *device_info,
                              bool         middle_button)
{
    XDevice *device;
    Atom prop;
    Atom type;
    int format, rc;
    unsigned long nitems, bytes_after;
    unsigned char *data;

    Display * display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
   // Display * display = QX11Info::display();
    prop = property_from_name ("Evdev Middle Button Emulation");
    if (!prop) /* no evdev devices */
        return;
    try {
        device = XOpenDevice (display, device_info->id);
        if (device == NULL)
            throw 1;
        rc = XGetDeviceProperty (display,
                                 device, prop, 0, 1, False, XA_INTEGER, &type, &format,
                                 &nitems, &bytes_after, &data);

        if (rc == Success && format == 8 && type == XA_INTEGER && nitems == 1) {
            data[0] = middle_button ? 1 : 0;
            XChangeDeviceProperty (display, device, prop, type, format, PropModeReplace, data, nitems);
        }
        if (rc == Success)
            XFree (data);
        XCloseDevice (display, device);
    } catch (int x) {
        qWarning("Error in setting middle button emulation on \"%s\"", device_info->name);
        return;
    }
}

void set_middle_button_libinput (XDeviceInfo *device_info,
                                 bool         middle_button)
{
    XDevice *device;
    Display *display = QX11Info::display();
    /* touchpad devices are excluded as the old code
     * only applies to evdev devices
     */
    device = device_is_touchpad (device_info);
    if (device != NULL) {
        try {
            XCloseDevice (display, device);
        } catch (int x) {
            return;
        }
    }
    try {
        device = XOpenDevice (display, device_info->id);
        if (device == NULL)
            throw 1;
        property_set_bool (device_info, device, "libinput Middle Emulation Enabled", 0, middle_button);
        XCloseDevice (display, device);
    } catch (int x) {
        USD_LOG(LOG_DEBUG,"%s:error id %d","MOUSE",x);
        return;
    }
}

void MouseManager::SetMiddleButton (XDeviceInfo *device_info,
                                    bool     middle_button)
{
    if (property_from_name ("Evdev Middle Button Emulation"))
        set_middle_button_evdev (device_info, middle_button);

    if (property_from_name ("libinput Middle Emulation Enabled"))
        set_middle_button_libinput (device_info, middle_button);
}

void MouseManager::SetMiddleButtonAll (bool middle_button)
{
    XDeviceInfo *device_info = nullptr;
    int n_devices;
    int i;
    Display * display = QX11Info::display();
    device_info = XListInputDevices (display, &n_devices);
    if(!device_info){
        qWarning("SetMiddleButtonAll: device_info is null");
        return;
    }
    for (i = 0; i < n_devices; i++) {
        SetMiddleButton (&device_info[i], middle_button);
    }

    if (device_info != NULL)
        XFreeDeviceList (device_info);
}

void MouseManager::SetLocatePointer (bool     state)
{
    if (state) {
        GError *error = NULL;
        char **args;
        int    argc;

        if (locate_pointer_spawned)
            return;
        QString str = "/usr/bin/usd-locate-pointer";
        if( g_shell_parse_argv (str.toLatin1().data(), &argc, &args, NULL)){
            g_spawn_async (g_get_home_dir (),
                           args,
                           NULL,
                           G_SPAWN_SEARCH_PATH,
                           NULL,
                           NULL,
                           &locate_pointer_pid,
                           &error);
            locate_pointer_spawned = (error == NULL);
        }
        if (error) {
            settings_mouse->set(KEY_MOUSE_LOCATE_POINTER,false);
            g_error_free (error);
        }
        g_strfreev (args);
    } else if (locate_pointer_spawned) {
            kill (locate_pointer_pid, SIGHUP);
            g_spawn_close_pid (locate_pointer_pid);
            locate_pointer_spawned = FALSE;
    }
}

void MouseManager::SetMouseWheelSpeed (int speed)
{
    if(speed <= 0 )
          return;
    GPid pid;
    QDir dir;
    QString FilePath = dir.homePath() + "/.imwheelrc";
    QFile file;
    int delay = 240000 / speed;
    QString date = QString("\".*\"\n"
                   "Control_L, Up,   Control_L|Button4\n"
                   "Control_R, Up,   Control_R|Button4\n"
                   "Control_L, Down, Control_L|Button5\n"
                   "Control_R, Down, Control_R|Button5\n"
                   "Shift_L,   Up,   Shift_L|Button4\n"
                   "Shift_R,   Up,   Shift_R|Button4\n"
                   "Shift_L,   Down, Shift_L|Button5\n"
                   "Shift_R,   Down, Shift_R|Button5\n"
                   "None,      Up,   Button4, %1, 0, %2\n"
                   "None,      Down, Button5, %3, 0, %4\n"
                   "None,      Thumb1,  Alt_L|Left\n"
                   "None,      Thumb2,  Alt_L|Right")
                .arg(speed).arg(delay).arg(speed).arg(delay);

    file.setFileName(FilePath);

    if(file.open(QIODevice::WriteOnly | QIODevice::Text)){
        file.write(date.toLatin1().data());
    }

    GError *error = NULL;
    char **args;
    int    argc;

//    if (imwheelSpawned){
//        QProcess::execute("killall imwheel");
//        imwheelSpawned = false;
//    }

    QString str = "/usr/bin/imwheel -k";
    if( g_shell_parse_argv (str.toLatin1().data(), &argc, &args, NULL)){
        g_spawn_async (g_get_home_dir (),
                       args,
                       NULL,
                       G_SPAWN_SEARCH_PATH,
                       NULL,
                       NULL,
                       &pid,
                       &error);
        imwheelSpawned = (error == NULL);
    }

    file.close();
    g_strfreev (args);
}

void MouseManager::MouseCallback (QString keys)
{
    if (keys.compare(QString::fromLocal8Bit(KEY_LEFT_HANDED))==0){
        bool mouse_left_handed = settings_mouse->get(keys).toBool();
        bool touchpad_left_handed = GetTouchpadHandedness (mouse_left_handed);
        SetLeftHandedAll (mouse_left_handed, touchpad_left_handed);

    } else if ((keys.compare(QString::fromLocal8Bit(KEY_MOTION_ACCELERATION))==0) ||
               (keys.compare(QString::fromLocal8Bit(KEY_MOTION_THRESHOLD))==0) ||
               (keys.compare(QString::fromLocal8Bit(KEY_MOUSE_ACCEL)) == 0)){
        SetMotionAll ();
    } else if (keys.compare(QString::fromLocal8Bit(KEY_MIDDLE_BUTTON_EMULATION))==0){
        SetMiddleButtonAll (settings_mouse->get(keys).toBool());

    } else if (keys.compare(QString::fromLocal8Bit(KEY_MOUSE_LOCATE_POINTER))==0){
        SetLocatePointer (settings_mouse->get(keys).toBool());
    } else if(keys.compare(QString::fromLocal8Bit(KEY_MOUSE_WHEEL_SPEED)) == 0 ) {
        SetMouseWheelSpeed (settings_mouse->get(keys).toInt());
    }else{
        USD_LOG(LOG_DEBUG,"keys:is skip..k%s", keys.toLatin1().data());
    }
}

gboolean have_program_in_path (const char *name)
{
    char *path;
    bool result;

    path = g_find_program_in_path (name);
    result = (path != NULL);
    g_free (path);
    return result;
}

void MouseManager::SetDisableWTypingSynaptics (bool state)
{
    if (state && touchpad_is_present ()) {
        GError *error = NULL;
        char **args;
        int    argc;
        QString cmd = "syndaemon -i 0.3 -K -R";

        if (syndaemon_spawned) {
            kill (syndaemon_pid, SIGHUP);
            g_spawn_close_pid (syndaemon_pid);
            syndaemon_spawned = FALSE;
            USD_LOG(LOG_DEBUG,"stop syndaemon");
        }


        if (!have_program_in_path ("syndaemon")) {
            return;
        }

        if (g_shell_parse_argv (cmd.toLatin1().data(), &argc, &args, NULL)) {
            g_spawn_async (g_get_home_dir (),
                           args,
                           NULL,
                           G_SPAWN_SEARCH_PATH,
                           NULL,
                           NULL,
                           &syndaemon_pid,
                           &error);
            syndaemon_spawned = (error == NULL);
        }
        USD_LOG(LOG_DEBUG,"start syndaemon(%d)", syndaemon_pid);
        if (error) {
                settings_touchpad->set(KEY_TOUCHPAD_DISABLE_W_TYPING,false);
                USD_LOG(LOG_ERR,"find error %s",error->message);
                g_error_free (error);

        }

        g_strfreev (args);

    } else if (syndaemon_spawned) {
        kill (syndaemon_pid, SIGHUP);
        g_spawn_close_pid (syndaemon_pid);
        syndaemon_spawned = FALSE;
    }
}
void touchpad_set_bool (XDeviceInfo *device_info,
                       const char  *property_name,
                       int          property_index,
                       bool          enabled)
{
    XDevice *device;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
            return;
    }
    property_set_bool (device_info, device, property_name, property_index, enabled);
    try {
        XCloseDevice (QX11Info::display(), device);
    } catch (int x) {
        qWarning("%s:error",__func__);
    }
}

void MouseManager::SetDisableWTypingLibinput (bool  state)
{
    XDeviceInfo *device_info = nullptr;
    int n_devices;
    int i;

    /* This is only called once for synaptics but for libinput
     * we need to loop through the list of devices
     */
    device_info = XListInputDevices (QX11Info::display(), &n_devices);
    if(!device_info){
        qWarning("SetDisableWTypingLibinput: device_info is null");
        return;
    }
    for (i = 0; i < n_devices; i++) {
        touchpad_set_bool (&device_info[i], "libinput Disable While Typing Enabled", 0, state);
    }

    if (device_info != NULL)
        XFreeDeviceList (device_info);
}

void MouseManager::SetDisableWTyping (bool state)
{
    if (property_from_name ("Synaptics Off"))
        SetDisableWTypingSynaptics (state);

    if (property_from_name ("libinput Disable While Typing Enabled"))
        SetDisableWTypingLibinput (state);
}

static void
set_tap_to_click_libinput (XDeviceInfo *device_info, bool   state)
{
        touchpad_set_bool (device_info, "libinput Tapping Enabled", 0, state);
}

static void
set_tap_to_click (XDeviceInfo *device_info,  bool state,  bool left_handed,
                  int one_finger_tap, int two_finger_tap, int three_finger_tap)
{
        if (property_from_name ("Synaptics Tap Action"))
                set_tap_to_click_synaptics (device_info, state, left_handed,
                                            one_finger_tap, two_finger_tap, three_finger_tap);

        if (property_from_name ("libinput Tapping Enabled"))
                set_tap_to_click_libinput (device_info, state);
}


void MouseManager::SetTapToClickAll ()
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);

    if (devicelist == NULL)
            return;

    bool state = settings_touchpad->get(KEY_TOUCHPAD_TAP_TO_CLICK).toBool();
    bool left_handed = GetTouchpadHandedness (settings_mouse->get(KEY_LEFT_HANDED).toBool());
    int one_finger_tap = settings_touchpad->get(KEY_TOUCHPAD_ONE_FINGER_TAP).toInt();
    int two_finger_tap = settings_touchpad->get(KEY_TOUCHPAD_TWO_FINGER_TAP).toInt();
    int three_finger_tap = settings_touchpad->get(KEY_TOUCHPAD_THREE_FINGER_TAP).toInt();

    for (i = 0; i < numdevices; i++) {
        set_tap_to_click (&devicelist[i], state, left_handed, one_finger_tap, two_finger_tap, three_finger_tap);
    }
    XFreeDeviceList (devicelist);
}

static void set_scrolling_synaptics (XDeviceInfo *device_info,
                                     QGSettings   *settings)
{
    touchpad_set_bool (device_info, "Synaptics Edge Scrolling", 0, settings->get(KEY_VERT_EDGE_SCROLL).toBool());
    touchpad_set_bool (device_info, "Synaptics Edge Scrolling", 1, settings->get(KEY_HORIZ_EDGE_SCROLL).toBool());
    touchpad_set_bool (device_info, "Synaptics Two-Finger Scrolling", 0, settings->get(KEY_VERT_TWO_FINGER_SCROLL).toBool());
    touchpad_set_bool (device_info, "Synaptics Two-Finger Scrolling", 1, settings->get(KEY_HORIZ_TWO_FINGER_SCROLL).toBool());
}


static void set_scrolling_libinput (XDeviceInfo *device_info,
                                    QGSettings   *settings)
{
    XDevice *device;
    int format, rc;
    unsigned long nitems, bytes_after;
    unsigned char *data;
    Atom prop, type;
    bool want_edge, want_2fg;
    bool want_horiz;
    //Display *display = QX11Info::display();
     Display * display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    prop = property_from_name ("libinput Scroll Method Enabled");
    if (!prop)
            return;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
            return;
    }

    want_2fg = settings->get(KEY_VERT_TWO_FINGER_SCROLL).toBool();
    want_edge  = settings->get(KEY_VERT_EDGE_SCROLL).toBool();

    /* libinput only allows for one scroll method at a time.
     * If both are set, pick 2fg scrolling.
     */
    if (want_2fg)
            want_edge = false;
    qDebug ("setting scroll method on %s", device_info->name);
    try {
        rc = XGetDeviceProperty (display, device, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3) {
                data[0] = want_2fg;
                data[1] = want_edge;
                XChangeDeviceProperty (display, device,
                                       prop, XA_INTEGER, 8, PropModeReplace, data, nitems);
        }
        if (rc == Success)
                XFree (data);
        XCloseDevice (display, device);
    } catch (int x) {
        qWarning("Error in setting scroll method on \"%s\"", device_info->name);
    }

    /* Horizontal scrolling is handled by xf86-input-libinput and
     * there's only one bool. Pick the one matching the scroll method
     * we picked above.
     */
    if (want_2fg)
        want_horiz = settings->get(KEY_HORIZ_TWO_FINGER_SCROLL).toBool();
    else if (want_edge)
        want_horiz = settings->get(KEY_HORIZ_EDGE_SCROLL).toBool();
    else
        return;
    touchpad_set_bool (device_info, "libinput Horizontal Scroll Enabled", 0, want_horiz);
}

static void set_scrolling (XDeviceInfo *device_info,
                           QGSettings   *settings)
 {
     if (property_from_name ("Synaptics Edge Scrolling"))
         set_scrolling_synaptics (device_info, settings);

     if (property_from_name ("libinput Scroll Method Enabled"))
         set_scrolling_libinput (device_info, settings);
 }

void SetScrollingAll (QGSettings *settings)
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);

    if (devicelist == NULL)
        return;

    for (i = 0; i < numdevices; i++) {
         set_scrolling (&devicelist[i], settings);
    }

    XFreeDeviceList (devicelist);
}

void set_natural_scroll_synaptics (XDeviceInfo *device_info,
                                   bool     natural_scroll)
{
    XDevice *device;
    int format, rc;
    unsigned long nitems, bytes_after;
    unsigned char* data;
    long *ptr;
    Atom prop, type;
     Display * display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
//    Display *display = QX11Info::display();
    prop = property_from_name ("Synaptics Scrolling Distance");
    if (!prop)
            return;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
            return;
    }

    qDebug ("Trying to set %s for \"%s\"",
            natural_scroll ? "natural (reverse) scroll" : "normal scroll",
            device_info->name);
    try {
        rc = XGetDeviceProperty (display , device, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 32 && nitems >= 2) {
                ptr = (glong *) data;
                if (natural_scroll) {
                        ptr[0] = -abs(ptr[0]);
                        ptr[1] = -abs(ptr[1]);
                } else {
                        ptr[0] = abs(ptr[0]);
                        ptr[1] = abs(ptr[1]);
                }

                XChangeDeviceProperty (display, device, prop,
                                       XA_INTEGER, 32, PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (display, device);
    } catch (int x) {
        qWarning("Error in setting natural scroll on \"%s\"", device_info->name);
    }
}

void set_natural_scroll_libinput (XDeviceInfo *device_info,
                                  bool       natural_scroll)
{
    USD_LOG (LOG_DEBUG,"Trying to set %s for \"%s\"",
            natural_scroll ? "natural (reverse) scroll" : "normal scroll",
            device_info->name);
    touchpad_set_bool (device_info, "libinput Natural Scrolling Enabled",
                       0, natural_scroll);
}


void set_natural_scroll (XDeviceInfo *device_info,
                         bool        natural_scroll)
{
    if (property_from_name ("Synaptics Scrolling Distance"))
        set_natural_scroll_synaptics (device_info, natural_scroll);

    if (property_from_name ("libinput Natural Scrolling Enabled"))
        set_natural_scroll_libinput (device_info, natural_scroll);
}

void MouseManager::SetNaturalScrollAll ()
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);

    if (devicelist == NULL)
            return;
    bool natural_scroll = settings_touchpad->get(KEY_TOUCHPAD_NATURAL_SCROLL).toBool();
    for (i = 0; i < numdevices; i++) {
            set_natural_scroll (&devicelist[i], natural_scroll);
    }
    XFreeDeviceList (devicelist);
}

void set_touchpad_enabled (XDeviceInfo *device_info,
                           bool         state)
{
    Display* display = gdk_x11_get_default_xdisplay ();
    XDevice * device;
    device = device_is_touchpad (device_info);
    if (device == NULL) {
        return;
    }
    int realformat;
    unsigned long nitems, bytes_after;
    unsigned char *data;
    Atom realtype, prop;
    prop = XInternAtom (display, "Device Enabled", False);
    if (!prop) {
        return;
    }

    if (XGetDeviceProperty (display, device, prop, 0, 1, False,
                            XA_INTEGER, &realtype, &realformat, &nitems,
                            &bytes_after, &data) == Success) {

        if (nitems == 1){
            data[0] = state ? 1 : 0;
            XChangeDeviceProperty(display, device, prop, XA_INTEGER, realformat, PropModeReplace, data, nitems);
        }
        XFree(data);
    }

    XCloseDevice (display, device);
}

void SetTouchpadEnabledAll (bool state)
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices ( QX11Info::display(), &numdevices);
    if (devicelist == NULL)
            return;
    for (i = 0; i < numdevices; i++) {
            set_touchpad_enabled (&devicelist[i], state);
    }
    XFreeDeviceList (devicelist);
}

bool SetDisbleTouchpad(XDeviceInfo *device_info,
                       QGSettings  *settings)
{
    QString name;
    bool   state;
    name = device_info->name;
    bool Pmouse = name.contains("Mouse", Qt::CaseInsensitive);
    bool Pusb = name.contains("USB", Qt::CaseInsensitive);
    if(Pmouse && Pusb){
        state = settings->get(KEY_TOUCHPAD_DISBLE_O_E_MOUSE).toBool();
        if(state){
            settings->set(KEY_TOUCHPAD_ENABLED, false);
            return true;
        }
    }
    return false;
}


// while remove mouse
void SetPlugRemoveMouseEnableTouchpad(QGSettings *settings)
{
    int numdevices, i;
    bool isMouse = false;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);
    if (devicelist == NULL){
        return;
    }
    for (i = 0; i < numdevices; i++) {
        QString name;
        name = devicelist[i].name;
        bool Pmouse = name.contains("Mouse", Qt::CaseInsensitive);
        bool Pusb = name.contains("USB", Qt::CaseInsensitive);
        if(Pmouse && Pusb) {
            isMouse = true;
        }
    }

    if(UsdBaseClass::isTablet()){
        if(settings->get(KEY_TOUCHPAD_ENABLED).toBool()) {
            settings->set(KEY_TOUCHPAD_ENABLED, true);
        }
    } else {
        if(!isMouse) {
            settings->set(KEY_TOUCHPAD_ENABLED, true);
        }
    }

    XFreeDeviceList (devicelist);
}

bool checkMouseExists()
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);
    if (devicelist == NULL){
        return false;
    }
    for (i = 0; i < numdevices; i++) {
        QString name;
        name = devicelist[i].name;
        bool Pmouse = name.contains("Mouse", Qt::CaseInsensitive);
        bool Pusb = name.contains("USB", Qt::CaseInsensitive);
        if(Pmouse && Pusb) {
            return true;
        }
    }
    XFreeDeviceList (devicelist);
    return false;
}

void SetPlugMouseDisbleTouchpad(QGSettings *settings)
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);
    if (devicelist == NULL) {
        return;
    }
    for (i = 0; i < numdevices; i++) {
        if(SetDisbleTouchpad (&devicelist[i], settings)) {
            break;
        }
    }
    XFreeDeviceList (devicelist);
}

void SetTouchpadDoubleClick(XDeviceInfo *device_info, bool state)
{
    XDevice *device;
    int format, rc;
    unsigned long nitems, bytes_after;
    unsigned char* data;
    Atom prop, type;
     Display * display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
//    Display *display = gdk_x11_get_default_xdisplay ();//QX11Info::display();
    prop = property_from_name ("Synaptics Gestures");
    if (!prop)
            return;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
            return;
    }
    qDebug ("Trying to set for \"%s\"", device_info->name);
    try {
        rc = XGetDeviceProperty (display , device, prop, 0, 1,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 1) {
            if(state)
                data[0]=1;
            else
                data[0]=0;

            XChangeDeviceProperty (display, device, prop,
                                   XA_INTEGER, 8, PropModeReplace, data, nitems);
        }
        if (rc == Success)
                XFree (data);
        XCloseDevice (display, device);
    } catch (int x) {
        qWarning("Error in setting natural scroll on \"%s\"", device_info->name);
    }
}
void SetTouchpadDoubleClickAll(bool state)
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);
    if (devicelist == NULL)
            return;
    for (i = 0; i < numdevices; i++) {
            SetTouchpadDoubleClick (&devicelist[i], state);
    }
    XFreeDeviceList (devicelist);
}
//设置关闭右下角菜单
void MouseManager::SetBottomRightClickMenu(XDeviceInfo *device_info, bool state)
{
    XDevice *device;
    int format, rc;
    unsigned long nitems, bytes_after;
    unsigned char* data;
    long *ptr;
    Atom prop, type;
     Display * display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
//    Display *display = gdk_x11_get_default_xdisplay ();//QX11Info::display();
    prop = property_from_name ("Synaptics Soft Button Areas");
    if (!prop)
            return;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
            return;
    }
    qDebug ("Trying to set for \"%s\"", device_info->name);
    try {
        rc = XGetDeviceProperty (display , device, prop, 0, 8,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 32 && nitems >= 3) {
            ptr = (long *)data;
            if(ptr[0] != 0){
                mAreaLeft = ptr[0];
                mAreaTop  = ptr[2];
            }
            if (state) {
                ptr[0] = mAreaLeft;
                ptr[2] = mAreaTop;
            } else {
                ptr[0] = 0;
                ptr[2] = 0;
            }

            XChangeDeviceProperty (display, device, prop,
                                   XA_INTEGER, 32, PropModeReplace, data, nitems);
        }
        if (rc == Success)
                XFree (data);
        XCloseDevice (display, device);
    } catch (int x) {
        qWarning("Error in setting natural scroll on \"%s\"", device_info->name);
    }
}

void MouseManager::SetBottomRightConrnerClickMenu(bool state)
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);
    if (devicelist == NULL)
        return;
    for (i = 0; i < numdevices; i++) {
        SetBottomRightClickMenu(&devicelist[i], state);
    }
    XFreeDeviceList (devicelist);
}

void MouseManager::TouchpadCallback (QString keys)
{
    if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_DISABLE_W_TYPING))==0) {
        SetDisableWTyping (settings_touchpad->get(keys).toBool());  //设置打字时禁用触摸板

    } else if (keys.compare(QString::fromLocal8Bit(KEY_LEFT_HANDED))== 0) {
        bool mouse_left_handed = settings_mouse->get(keys).toBool();
        bool touchpad_left_handed = GetTouchpadHandedness (mouse_left_handed);
        SetLeftHandedAll (mouse_left_handed, touchpad_left_handed); //设置左右手

    } else if ((keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_TAP_TO_CLICK))    == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_ONE_FINGER_TAP))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_TWO_FINGER_TAP))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_THREE_FINGER_TAP))== 0)) {
        SetTapToClickAll ();                                        //设置多指手势

    } else if ((keys.compare(QString::fromLocal8Bit(KEY_VERT_EDGE_SCROLL))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_HORIZ_EDGE_SCROLL)) == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_VERT_TWO_FINGER_SCROLL))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_HORIZ_TWO_FINGER_SCROLL)) == 0)) {
        SetScrollingAll (settings_touchpad);                //设置滚动

    } else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_NATURAL_SCROLL)) == 0) {
        SetNaturalScrollAll ();                             //设置上移下滚或上移上滚
        USD_LOG(LOG_DEBUG,"set %s",KEY_TOUCHPAD_NATURAL_SCROLL);

    } else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_ENABLED)) == 0) {
        SetTouchpadEnabledAll (settings_touchpad->get(keys).toBool());//设置触摸板开关
    } else if ((keys.compare((KEY_MOTION_ACCELERATION)) == 0)
            || (keys.compare((KEY_MOTION_THRESHOLD)) == 0)) {
        SetMotionAll ();                                    //设置鼠标速度
    }else if (0 == QString::compare(keys, QString(KEY_MOTION_ACCELERATION), Qt::CaseInsensitive)||
              0 == QString::compare(keys, QString(KEY_MOTION_THRESHOLD), Qt::CaseInsensitive)){
        SetMotionAll ();                                    //设置鼠标速度

    }else if (keys == "motion-acceleration" || keys==KEY_MOTION_THRESHOLD){
    }else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_DISBLE_O_E_MOUSE)) == 0) {
        SetPlugMouseDisbleTouchpad(settings_touchpad);      //设置插入鼠标时禁用触摸板

    } else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_DOUBLE_CLICK_DRAG)) == 0){
        SetTouchpadDoubleClickAll(settings_touchpad->get(KEY_TOUCHPAD_DOUBLE_CLICK_DRAG).toBool());//设置轻点击两次拖动打开关闭

    } else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_BOTTOM_R_C_CLICK_M)) == 0){
        SetBottomRightConrnerClickMenu(settings_touchpad->get(KEY_TOUCHPAD_BOTTOM_R_C_CLICK_M).toBool());//打开关闭右下角点击弹出菜单

    } else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_MOUSE_SENSITVITY)) == 0){

    } else {
        USD_LOG(LOG_DEBUG,"keys:is skip..k%s", keys.toLatin1().data(),keys.toLatin1().data());
    }
}

void MouseManager::SetMouseSettings ()
{
    bool mouse_left_handed = settings_mouse->get(KEY_LEFT_HANDED).toBool();
    bool touchpad_left_handed = GetTouchpadHandedness (mouse_left_handed);

    SetLeftHandedAll (mouse_left_handed, touchpad_left_handed);

    SetMotionAll ();
    SetMiddleButtonAll (settings_mouse->get(KEY_MIDDLE_BUTTON_EMULATION).toBool());
    SetMouseWheelSpeed (settings_mouse->get(KEY_MOUSE_WHEEL_SPEED).toInt());
    SetPlugMouseDisbleTouchpad(settings_touchpad);

}

void MouseManager::SetTouchSettings ()
{
    SetTapToClickAll ();
    SetScrollingAll (settings_touchpad);
    SetNaturalScrollAll ();
    SetTouchpadEnabledAll (settings_touchpad->get(KEY_TOUCHPAD_ENABLED).toBool());
//    SetPlugMouseDisbleTouchpad(settings_touchpad);
    SetPlugRemoveMouseEnableTouchpad(settings_touchpad);
    SetTouchpadDoubleClickAll(settings_touchpad->get(KEY_TOUCHPAD_DOUBLE_CLICK_DRAG).toBool());
    SetBottomRightConrnerClickMenu(settings_touchpad->get(KEY_TOUCHPAD_BOTTOM_R_C_CLICK_M).toBool());

}

GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
                                       GdkEvent  *event,
                                       gpointer   data)
{
    XEvent *xev = (XEvent *) xevent;
    XEventClass class_presence;
    int xi_presence;
    MouseManager * manager = (MouseManager *) data;

    DevicePresence (gdk_x11_get_default_xdisplay (), xi_presence, class_presence);
    if (xev->type == xi_presence)
    {
            XDevicePresenceNotifyEvent *dpn = (XDevicePresenceNotifyEvent *) xev;
            if (dpn->devchange == DeviceEnabled) {
                USD_LOG(LOG_DEBUG,"new add deviced ID  : %d",dpn->deviceid);
                manager->SetMouseSettings ();
            } else if(dpn->devchange == DeviceRemoved) {
                manager->SetTouchSettings();
            }
    }
    return GDK_FILTER_CONTINUE;
}

void MouseManager::SetDevicepresenceHandler ()
{
    Display *display;
    XEventClass class_presence;
    int xi_presence;
    display = QX11Info::display();

    gdk_x11_display_error_trap_push (gdk_display_get_default());
    DevicePresence (display, xi_presence, class_presence);
    XSelectExtensionEvent (display,
                           RootWindow (display, DefaultScreen (display)),
                           &class_presence, 1);

    gdk_display_flush (gdk_display_get_default());
    if (!gdk_x11_display_error_trap_pop (gdk_display_get_default()))
        gdk_window_add_filter (NULL, devicepresence_filter,this);
}

void MouseManager::MouseManagerIdleCb()
{

    time->stop();

//    connect(settings_mouse, &QGSettings::changed, this, &MouseManager::MouseCallback);
//    connect(settings_touchpad, &QGSettings::changed, this, &MouseManager::TouchpadCallback);

    QObject::connect(settings_mouse,SIGNAL(changed(QString)),  this,SLOT(MouseCallback(QString)));
    QObject::connect(settings_touchpad,SIGNAL(changed(QString)),this,SLOT(TouchpadCallback(QString)));

    syndaemon_spawned = FALSE;

    SetDevicepresenceHandler ();
    SetMouseSettings ();
    SetTouchSettings ();
    SetDisableWTyping (settings_touchpad->get(KEY_TOUCHPAD_DISABLE_W_TYPING).toBool());
    SetLocatePointer (settings_mouse->get(KEY_MOUSE_LOCATE_POINTER).toBool());
    if(checkMouseExists()){
        SetPlugMouseDisbleTouchpad(settings_touchpad);
    } else {
        SetPlugRemoveMouseEnableTouchpad(settings_touchpad);
    }
}
