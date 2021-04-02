/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"

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

#include "ukui-settings-profile.h"
#include "usd-mouse-manager.h"
#include "usd-input-helper.h"

#define USD_MOUSE_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), USD_TYPE_MOUSE_MANAGER, UsdMouseManagerPrivate))

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
#define KEY_TOUCHPAD_DOUBLE_CLICK_DRAG   "double-click-drag"                //点击两次拖动     true/false
#define KEY_TOUCHPAD_BOTTOM_R_C_CLICK_M  "bottom-right-corner-click-menu"   //右下角点击菜单   true/false

#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
#define UKUI_MOUSE_A11Y_SCHEMA           "org.ukui.accessibility-mouse"
#define KEY_MOUSE_A11Y_DWELL_ENABLE      "dwell-enable"
#define KEY_MOUSE_A11Y_DELAY_ENABLE      "delay-enable"
#endif

struct UsdMouseManagerPrivate
{
        GSettings *settings_mouse;
        GSettings *settings_touchpad;
        unsigned long mAreaLeft;
        unsigned long mAreaTop;

#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
        gboolean mousetweaks_daemon_running;
#endif
        gboolean syndaemon_spawned;
        GPid syndaemon_pid;
        gboolean locate_pointer_spawned;
        GPid locate_pointer_pid;
        gboolean imwheel_spawned;
};

typedef enum {
        TOUCHPAD_HANDEDNESS_RIGHT,
        TOUCHPAD_HANDEDNESS_LEFT,
        TOUCHPAD_HANDEDNESS_MOUSE
} TouchpadHandedness;

static void     usd_mouse_manager_class_init  (UsdMouseManagerClass *klass);
static void     usd_mouse_manager_init        (UsdMouseManager      *mouse_manager);
static void     usd_mouse_manager_finalize    (GObject             *object);
static void     set_mouse_settings            (UsdMouseManager      *manager);
static void     set_tap_to_click_synaptics    (XDeviceInfo          *device_info,
                                               gboolean              state,
                                               gboolean              left_handed,
                                               gint                  one_finger_tap,
                                               gint                  two_finger_tap,
                                               gint                  three_finger_tap);


G_DEFINE_TYPE (UsdMouseManager, usd_mouse_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static void
usd_mouse_manager_class_init (UsdMouseManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = usd_mouse_manager_finalize;

        g_type_class_add_private (klass, sizeof (UsdMouseManagerPrivate));
}

static void
configure_button_layout (guchar   *buttons,
                         gint      n_buttons,
                         gboolean  left_handed)
{
        const gint left_button = 1;
        gint right_button;
        gint i;

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

static gboolean
xinput_device_has_buttons (XDeviceInfo *device_info)
{
        int i;
        XAnyClassInfo *class_info;

        class_info = device_info->inputclassinfo;
        for (i = 0; i < device_info->num_classes; i++) {
                if (class_info->class == ButtonClass) {
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

static Atom
property_from_name (const char *property_name)
{
        return XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), property_name, True);
}

static gboolean
touchpad_has_single_button (XDevice *device)
{
        Atom type, prop;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        gboolean is_single_button = FALSE;
        int rc;

        prop = property_from_name ("Synaptics Capabilities");
        if (!prop)
                return FALSE;

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 1, False,
                                 XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);
        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3)
                is_single_button = (data[0] == 1 && data[1] == 0 && data[2] == 0);

        if (rc == Success)
                XFree (data);

        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default ());

        return is_single_button;
}

static gboolean
property_exists_on_device (XDeviceInfo *device_info,
                           const char  *property_name)
{
        XDevice *device;
        int rc;
        Atom type, prop;
        int format;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        prop = property_from_name (property_name);
        if (!prop)
                return FALSE;

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) || (device == NULL))
                return FALSE;

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                 device, prop, 0, 1, False, XA_INTEGER, &type, &format,
                                 &nitems, &bytes_after, &data);

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default ());

        return rc == Success;
}

static void
property_set_bool (XDeviceInfo *device_info,
                   XDevice     *device,
                   const char  *property_name,
                   int          property_index,
                   gboolean     enabled)
{
        int rc;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        int act_format;
        Atom act_type, property;

        property = property_from_name (property_name);
        if (!property)
                return;

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device,
                                 property, 0, 1, False,
                                 XA_INTEGER, &act_type, &act_format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && act_type == XA_INTEGER && act_format == 8 && nitems > property_index) {
                data[property_index] = enabled ? 1 : 0;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device,
                                       property, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error while setting %s on \"%s\"", property_name, device_info->name);
        }
}

static void
touchpad_set_bool (XDeviceInfo *device_info,
                   const char  *property_name,
                   int          property_index,
                   gboolean     enabled)
{
        XDevice *device;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                return;
        }
        property_set_bool (device_info, device, property_name, property_index, enabled);

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default ());
}

static void
set_left_handed_legacy_driver (UsdMouseManager *manager,
                               XDeviceInfo     *device_info,
                               gboolean         mouse_left_handed,
                               gboolean         touchpad_left_handed)
{
        XDevice *device;
        guchar *buttons;
        gsize buttons_capacity = 16;
        gint n_buttons;
        gboolean left_handed;

        if ((device_info->use == IsXPointer) ||
            (device_info->use == IsXKeyboard) ||
            (g_strcmp0 ("Virtual core XTEST pointer", device_info->name) == 0) ||
            (!xinput_device_has_buttons (device_info)))
                return;

        /* If the device is a touchpad, swap tap buttons
         * around too, otherwise a tap would be a right-click */
        device = device_is_touchpad (device_info);
        if (device != NULL) {
                gboolean tap = g_settings_get_boolean (manager->priv->settings_touchpad, KEY_TOUCHPAD_TAP_TO_CLICK);
                gboolean single_button = touchpad_has_single_button (device);

                left_handed = touchpad_left_handed;

                if (tap && !single_button) {
                        gint one_finger_tap = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_ONE_FINGER_TAP);
                        gint two_finger_tap = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_TWO_FINGER_TAP);
                        gint three_finger_tap = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_THREE_FINGER_TAP);
                        set_tap_to_click_synaptics (device_info, tap, left_handed, one_finger_tap, two_finger_tap, three_finger_tap);
                }

                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);

                if (single_button)
                        return;
        } else {
                left_handed = mouse_left_handed;
        }

        gdk_x11_display_error_trap_push (gdk_display_get_default ());

        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) ||
            (device == NULL))
                return;

        buttons = g_new (guchar, buttons_capacity);

        n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device,
                                             buttons,
                                             buttons_capacity);

        while (n_buttons > buttons_capacity) {
                buttons_capacity = n_buttons;
                buttons = (guchar *) g_realloc (buttons,
                                                buttons_capacity * sizeof (guchar));

                n_buttons = XGetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device,
                                                     buttons,
                                                     buttons_capacity);
        }

        configure_button_layout (buttons, n_buttons, left_handed);

        XSetDeviceButtonMapping (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, buttons, n_buttons);
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);

        g_free (buttons);
}

static void
set_left_handed_libinput (XDeviceInfo *device_info,
                          gboolean     mouse_left_handed,
                          gboolean     touchpad_left_handed)
{
        XDevice *device;
        gboolean want_lefthanded;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                gdk_x11_display_error_trap_push (gdk_display_get_default ());
                device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
                if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) || (device == NULL))
                        return;

                want_lefthanded = mouse_left_handed;
        } else {
                /* touchpad device is already open after
                 * return from device_is_touchpad function
                 */
                want_lefthanded = touchpad_left_handed;
        }

        property_set_bool (device_info, device, "libinput Left Handed Enabled", 0, want_lefthanded);

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default ());
}

static void
set_left_handed (UsdMouseManager *manager,
                 XDeviceInfo     *device_info,
                 gboolean         mouse_left_handed,
                 gboolean         touchpad_left_handed)
{
        if (property_exists_on_device (device_info, "libinput Left Handed Enabled"))
                set_left_handed_libinput (device_info, mouse_left_handed, touchpad_left_handed);
        else
                set_left_handed_legacy_driver (manager, device_info, mouse_left_handed, touchpad_left_handed);
}

static void
set_left_handed_all (UsdMouseManager *manager,
                     gboolean         mouse_left_handed,
                     gboolean         touchpad_left_handed)
{
        XDeviceInfo *device_info;
        gint n_devices;
        gint i;

        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);

        for (i = 0; i < n_devices; i++) {
                set_left_handed (manager, &device_info[i], mouse_left_handed, touchpad_left_handed);
        }

        if (device_info != NULL)
                XFreeDeviceList (device_info);
}

static GdkFilterReturn
devicepresence_filter (GdkXEvent *xevent,
                       GdkEvent  *event,
                       gpointer   data)
{
        XEvent *xev = (XEvent *) xevent;
        XEventClass class_presence;
        int xi_presence;

        DevicePresence (gdk_x11_get_default_xdisplay (), xi_presence, class_presence);

        if (xev->type == xi_presence)
        {
                XDevicePresenceNotifyEvent *dpn = (XDevicePresenceNotifyEvent *) xev;
                if (dpn->devchange == DeviceEnabled)
                        set_mouse_settings ((UsdMouseManager *) data);
        }

        return GDK_FILTER_CONTINUE;
}

static void
set_devicepresence_handler (UsdMouseManager *manager)
{
        Display *display;
        XEventClass class_presence;
        int xi_presence;

        display = gdk_x11_get_default_xdisplay ();

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        DevicePresence (display, xi_presence, class_presence);
        XSelectExtensionEvent (display,
                               RootWindow (display, DefaultScreen (display)),
                               &class_presence, 1);

        gdk_flush ();
        if (!gdk_x11_display_error_trap_pop (gdk_display_get_default ()))
                gdk_window_add_filter (NULL, devicepresence_filter, manager);
}

static void
set_motion_legacy_driver (UsdMouseManager *manager,
                          XDeviceInfo     *device_info)
{
        XDevice *device;
        XPtrFeedbackControl feedback;
        XFeedbackState *states, *state;
        gint num_feedbacks, i;
        GSettings *settings;
        gdouble motion_acceleration;
        gint motion_threshold;
        gint numerator, denominator;

        device = device_is_touchpad (device_info);
        if (device != NULL) {
                settings = manager->priv->settings_touchpad;
        } else {
                gdk_x11_display_error_trap_push (gdk_display_get_default ());
                device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
                if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) || (device == NULL))
                        return;

                settings = manager->priv->settings_mouse;
        }

        /* Calculate acceleration */
        motion_acceleration = g_settings_get_double (settings, KEY_MOTION_ACCELERATION);
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
        motion_threshold = g_settings_get_int (settings, KEY_MOTION_THRESHOLD);

        /* Get the list of feedbacks for the device */
        states = XGetFeedbackControl (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, &num_feedbacks);
        if (states == NULL) {
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
                return;
        }

        state = (XFeedbackState *) states;
        for (i = 0; i < num_feedbacks; i++) {
                if (state->class == PtrFeedbackClass) {
                        /* And tell the device */
                        feedback.class      = PtrFeedbackClass;
                        feedback.length     = sizeof (XPtrFeedbackControl);
                        feedback.id         = state->id;
                        feedback.threshold  = motion_threshold;
                        feedback.accelNum   = numerator;
                        feedback.accelDenom = denominator;

                        g_debug ("Setting accel %d/%d, threshold %d for device '%s'",
                                 numerator, denominator, motion_threshold, device_info->name);

                        XChangeFeedbackControl (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                                device,
                                                DvAccelNum | DvAccelDenom | DvThreshold,
                                                (XFeedbackControl *) &feedback);
                        break;
                }

                state = (XFeedbackState *) ((char *) state + state->length);
        }

        XFreeFeedbackList (states);
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
}

static void
set_motion_libinput (UsdMouseManager *manager,
                     XDeviceInfo     *device_info)
{
        XDevice *device;
        Atom prop;
        Atom type;
        Atom float_type;
        int format, rc;
        unsigned long nitems, bytes_after;
        GSettings *settings;
        union {
                unsigned char *c;
                long *l;
        } data;
        gfloat accel;
        gfloat motion_acceleration;

        float_type = property_from_name ("FLOAT");
        if (!float_type)
                return;

        prop = property_from_name ("libinput Accel Speed");
        if (!prop) {
                return;
        }

        device = device_is_touchpad (device_info);
        if (device != NULL) {
                settings = manager->priv->settings_touchpad;
        } else {
                gdk_x11_display_error_trap_push (gdk_display_get_default ());
                device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
                if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) || (device == NULL))
                        return;

                settings = manager->priv->settings_mouse;
        }

        /* Calculate acceleration */
        motion_acceleration = g_settings_get_double (settings, KEY_MOTION_ACCELERATION);

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

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                 device, prop, 0, 1, False, float_type, &type, &format,
                                 &nitems, &bytes_after, &data.c);

        if (rc == Success && type == float_type && format == 32 && nitems >= 1) {
                *(float *) data.l = accel;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                       device, prop, float_type, 32, PropModeReplace, data.c, nitems);
        }

        if (rc == Success) {
                XFree (data.c);
        }

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error while setting accel speed on \"%s\"", device_info->name);
        }
}

static void
set_mouse_accel (UsdMouseManager *manager,
                 XDeviceInfo     *device_info)
{
    XDevice *device;
    Atom prop;
    Atom type;
    int format, rc;
    unsigned long nitems, bytes_after;
    GSettings *settings;

    Display * dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    unsigned char *data;
    gboolean mouse_accel;

    prop = property_from_name ("libinput Accel Profile Enabled");
    if (!prop) {
        return;
    }
    gdk_x11_display_error_trap_push (gdk_display_get_default ());
    device = XOpenDevice (dpy, device_info->id);
    if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) || (device == NULL))
        return;

    gdk_x11_display_error_trap_push (gdk_display_get_default ());
    rc = XGetDeviceProperty (dpy,device, prop, 0, 2, False, XA_INTEGER, &type,
                            &format, &nitems, &bytes_after, &data);
    if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 1) {
        settings = manager->priv->settings_mouse;
        mouse_accel = g_settings_get_boolean (settings, KEY_MOUSE_ACCEL);
        if(mouse_accel){
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
    if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
        g_warning ("Error while setting mouse accel on \"%s\"", device_info->name);
    }
}

static void 
set_touchpad_motion_accel(XDeviceInfo *device_info, GSettings *settings)
{
        XDevice *device;
        Atom prop;
        Atom type;
        Atom float_type;
        int format, rc;
        unsigned long nitems, bytes_after;

        Display * dpy = gdk_x11_get_default_xdisplay ();
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
        device = device_is_touchpad (device_info);
        if (device == NULL)
            return;
        /* Calculate acceleration */
        motion_acceleration = g_settings_get_double (settings, KEY_MOTION_ACCELERATION);
        if (motion_acceleration == -1.0) /* unset */
                accel = 0.0;
        else
                accel = motion_acceleration;
        
        gdk_x11_display_error_trap_push (gdk_display_get_default ());
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
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
            g_warning ("Error while setting touchpad accel on \"%s\"", device_info->name);
        }
}

static void
set_motion (UsdMouseManager *manager,
            XDeviceInfo     *device_info)
{
        if (property_exists_on_device (device_info, "libinput Accel Speed"))
                set_motion_libinput (manager, device_info);
        else
                set_motion_legacy_driver (manager, device_info);
        
        if(property_exists_on_device (device_info, "Device Accel Constant Deceleration"))
                set_touchpad_motion_accel(device_info, manager->priv->settings_touchpad);

        if(property_exists_on_device (device_info, "libinput Accel Profile Enabled"))
              set_mouse_accel(manager, device_info);
}

static void
set_motion_all (UsdMouseManager *manager)
{
        XDeviceInfo *device_info;
        gint n_devices;
        gint i;

        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);

        for (i = 0; i < n_devices; i++) {
                set_motion (manager, &device_info[i]);
        }

        if (device_info != NULL)
                XFreeDeviceList (device_info);
}

static void
set_middle_button_evdev (XDeviceInfo *device_info,
                         gboolean     middle_button)
{
        XDevice *device;
        Atom prop;
        Atom type;
        int format, rc;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        prop = property_from_name ("Evdev Middle Button Emulation");

        if (!prop) /* no evdev devices */
                return;

        gdk_x11_display_error_trap_push (gdk_display_get_default ());

        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) ||
            (device == NULL))
                return;

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                 device, prop, 0, 1, False, XA_INTEGER, &type, &format,
                                 &nitems, &bytes_after, &data);

        if (rc == Success && format == 8 && type == XA_INTEGER && nitems == 1) {
                data[0] = middle_button ? 1 : 0;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                       device, prop, type, format, PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error in setting middle button emulation on \"%s\"", device_info->name);
        }
}

static void
set_middle_button_libinput (XDeviceInfo *device_info,
                            gboolean     middle_button)
{
        XDevice *device;

        /* touchpad devices are excluded as the old code
         * only applies to evdev devices
         */
        device = device_is_touchpad (device_info);
        if (device != NULL) {
                gdk_x11_display_error_trap_push (gdk_display_get_default ());
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
                gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default ());
                return;
        }

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_x11_display_error_trap_pop (gdk_display_get_default ()) != 0) || (device == NULL))
                return;

        property_set_bool (device_info, device, "libinput Middle Emulation Enabled", 0, middle_button);

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default ());
}

static void
set_middle_button (XDeviceInfo *device_info,
                   gboolean     middle_button)
{
        if (property_from_name ("Evdev Middle Button Emulation"))
                set_middle_button_evdev (device_info, middle_button);

        if (property_from_name ("libinput Middle Emulation Enabled"))
                set_middle_button_libinput (device_info, middle_button);
}

static void
set_middle_button_all (gboolean middle_button)
{
        XDeviceInfo *device_info;
        gint n_devices;
        gint i;

        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);

        for (i = 0; i < n_devices; i++) {
                set_middle_button (&device_info[i], middle_button);
        }

        if (device_info != NULL)
                XFreeDeviceList (device_info);
}

static gboolean
have_program_in_path (const char *name)
{
        gchar *path;
        gboolean result;

        path = g_find_program_in_path (name);
        result = (path != NULL);
        g_free (path);
        return result;
}

static void
set_disable_w_typing_synaptics (UsdMouseManager *manager,
                                gboolean         state)
{
        if (state && touchpad_is_present ()) {
                GError *error = NULL;
                char *args[6];

                if (manager->priv->syndaemon_spawned)
                        return;

                args[0] = "syndaemon";
                args[1] = "-i";
                args[2] = "0.5";
                args[3] = "-K";
                args[4] = "-R";
                args[5] = NULL;

                if (!have_program_in_path (args[0]))
                        return;

                g_spawn_async (g_get_home_dir (), args, NULL,
                                G_SPAWN_SEARCH_PATH, NULL, NULL,
                                &manager->priv->syndaemon_pid, &error);

                manager->priv->syndaemon_spawned = (error == NULL);

                if (error) {
                        g_settings_set_boolean (manager->priv->settings_touchpad, KEY_TOUCHPAD_DISABLE_W_TYPING, FALSE);
                        g_error_free (error);
                }

        } else if (manager->priv->syndaemon_spawned)
        {
                kill (manager->priv->syndaemon_pid, SIGHUP);
                g_spawn_close_pid (manager->priv->syndaemon_pid);
                manager->priv->syndaemon_spawned = FALSE;
        }
}

static void
set_disable_w_typing_libinput (UsdMouseManager *manager,
                               gboolean         state)
{
        XDeviceInfo *device_info;
        gint n_devices;
        gint i;

        /* This is only called once for synaptics but for libinput
         * we need to loop through the list of devices
         */
        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);

        for (i = 0; i < n_devices; i++) {
                touchpad_set_bool (&device_info[i], "libinput Disable While Typing Enabled", 0, state);
        }

        if (device_info != NULL)
                XFreeDeviceList (device_info);
}

static void
set_disable_w_typing (UsdMouseManager *manager,
                      gboolean         state)
{
        if (property_from_name ("Synaptics Off"))
                set_disable_w_typing_synaptics (manager, state);

        if (property_from_name ("libinput Disable While Typing Enabled"))
                set_disable_w_typing_libinput (manager, state);
}

static gboolean
get_touchpad_handedness (UsdMouseManager *manager,
                         gboolean         mouse_left_handed)
{
        switch (g_settings_get_enum (manager->priv->settings_touchpad, KEY_LEFT_HANDED)) {
        case TOUCHPAD_HANDEDNESS_RIGHT:
                return FALSE;
        case TOUCHPAD_HANDEDNESS_LEFT:
                return TRUE;
        case TOUCHPAD_HANDEDNESS_MOUSE:
                return mouse_left_handed;
        default:
                g_assert_not_reached ();
        }
}

static void
set_tap_to_click_synaptics (XDeviceInfo *device_info,
                            gboolean     state,
                            gboolean     left_handed,
                            gint         one_finger_tap,
                            gint         two_finger_tap,
                            gint         three_finger_tap)
{
        XDevice *device;
        int format, rc;
        unsigned long nitems, bytes_after;
        unsigned char* data;
        Atom prop, type;

        prop = property_from_name ("Synaptics Tap Action");

        if (!prop)
                return;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                return;
        }

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (one_finger_tap > 3 || one_finger_tap < 1)
                one_finger_tap = 1;
        if (two_finger_tap > 3 || two_finger_tap < 1)
                two_finger_tap = 3;
        if (three_finger_tap > 3 || three_finger_tap < 1)
                three_finger_tap = 2;

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 7)
        {
                /* Set RLM mapping for 1/2/3 fingers*/
                data[4] = (state) ? ((left_handed) ? (4-one_finger_tap) : one_finger_tap) : 0;
                data[5] = (state) ? ((left_handed) ? (4-two_finger_tap) : two_finger_tap) : 0;
                data[6] = (state) ? three_finger_tap : 0;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, XA_INTEGER, 8,
                                       PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error in setting tap to click on \"%s\"", device_info->name);
        }
}

static void
set_tap_to_click_libinput (XDeviceInfo *device_info,
                           gboolean     state)
{
        touchpad_set_bool (device_info, "libinput Tapping Enabled", 0, state);
}

static void
set_tap_to_click (XDeviceInfo *device_info,
                  gboolean     state,
                  gboolean     left_handed,
                  gint         one_finger_tap,
                  gint         two_finger_tap,
                  gint         three_finger_tap)
{
        if (property_from_name ("Synaptics Tap Action"))
                set_tap_to_click_synaptics (device_info, state, left_handed,
                                            one_finger_tap, two_finger_tap, three_finger_tap);

        if (property_from_name ("libinput Tapping Enabled"))
                set_tap_to_click_libinput (device_info, state);
}

static void
set_tap_to_click_all (UsdMouseManager *manager)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        gboolean state = g_settings_get_boolean (manager->priv->settings_touchpad, KEY_TOUCHPAD_TAP_TO_CLICK);
        gboolean left_handed = get_touchpad_handedness (manager, g_settings_get_boolean (manager->priv->settings_mouse, KEY_LEFT_HANDED));
        gint one_finger_tap = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_ONE_FINGER_TAP);
        gint two_finger_tap = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_TWO_FINGER_TAP);
        gint three_finger_tap = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_THREE_FINGER_TAP);

        for (i = 0; i < numdevices; i++) {
                set_tap_to_click (&devicelist[i], state, left_handed, one_finger_tap, two_finger_tap, three_finger_tap);
        }

        XFreeDeviceList (devicelist);
}

static void
set_click_actions_synaptics (XDeviceInfo *device_info,
                             gint         enable_two_finger_click,
                             gint         enable_three_finger_click)
{
        XDevice *device;
        int format, rc;
        unsigned long nitems, bytes_after;
        unsigned char* data;
        Atom prop, type;

        prop = property_from_name ("Synaptics Click Action");
        if (!prop)
                return;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                return;
        }

        g_debug ("setting click action to click on %s", device_info->name);

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3) {
                data[0] = 1;
                data[1] = enable_two_finger_click;
                data[2] = enable_three_finger_click;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop,
                                       XA_INTEGER, 8, PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error in setting click actions on \"%s\"", device_info->name);
        }
}

static void
set_click_actions_libinput (XDeviceInfo *device_info,
                            gint         enable_two_finger_click,
                            gint         enable_three_finger_click)
{
        XDevice *device;
        int format, rc;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        Atom prop, type;
        gboolean want_clickfinger;
        gboolean want_softwarebuttons;

        prop = property_from_name ("libinput Click Method Enabled");
        if (!prop)
                return;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                return;
        }

        g_debug ("setting click action to click on %s", device_info->name);

        want_clickfinger = enable_two_finger_click || enable_three_finger_click;
        want_softwarebuttons = !want_clickfinger;

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 2) {
                data[0] = want_softwarebuttons;
                data[1] = want_clickfinger;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop,
                                       XA_INTEGER, 8, PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error in setting click actions on \"%s\"", device_info->name);
        }
}

static void
set_click_actions (XDeviceInfo *device_info,
                   gint         enable_two_finger_click,
                   gint         enable_three_finger_click)
{
        if (property_from_name ("Synaptics Click Action"))
                set_click_actions_synaptics (device_info, enable_two_finger_click, enable_three_finger_click);

        if (property_from_name ("libinput Click Method Enabled"))
                set_click_actions_libinput (device_info, enable_two_finger_click, enable_three_finger_click);
}

static void
set_click_actions_all (UsdMouseManager *manager)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        gint enable_two_finger_click = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_TWO_FINGER_CLICK);
        gint enable_three_finger_click = g_settings_get_int (manager->priv->settings_touchpad, KEY_TOUCHPAD_THREE_FINGER_CLICK);

        for (i = 0; i < numdevices; i++) {
                set_click_actions (&devicelist[i], enable_two_finger_click, enable_three_finger_click);
        }

        XFreeDeviceList (devicelist);
}

static void
set_natural_scroll_synaptics (XDeviceInfo *device_info,
                              gboolean     natural_scroll)
{
        XDevice *device;
        int format, rc;
        unsigned long nitems, bytes_after;
        unsigned char* data;
        glong *ptr;
        Atom prop, type;

        prop = property_from_name ("Synaptics Scrolling Distance");
        if (!prop)
                return;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                return;
        }

        g_debug ("Trying to set %s for \"%s\"", natural_scroll ? "natural (reverse) scroll" : "normal scroll", device_info->name);

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 2,
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

                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop,
                                       XA_INTEGER, 32, PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error in setting natural scroll on \"%s\"", device_info->name);
        }
}

static void
set_natural_scroll_libinput (XDeviceInfo *device_info,
                             gboolean     natural_scroll)
{
        g_debug ("Trying to set %s for \"%s\"", natural_scroll ? "natural (reverse) scroll" : "normal scroll", device_info->name);

        touchpad_set_bool (device_info, "libinput Natural Scrolling Enabled", 0, natural_scroll);
}

static void
set_natural_scroll (XDeviceInfo *device_info,
                    gboolean     natural_scroll)
{
        if (property_from_name ("Synaptics Scrolling Distance"))
                set_natural_scroll_synaptics (device_info, natural_scroll);

        if (property_from_name ("libinput Natural Scrolling Enabled"))
                set_natural_scroll_libinput (device_info, natural_scroll);
}

static void
set_natural_scroll_all (UsdMouseManager *manager)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        gboolean natural_scroll = g_settings_get_boolean (manager->priv->settings_touchpad, KEY_TOUCHPAD_NATURAL_SCROLL);

        for (i = 0; i < numdevices; i++) {
                set_natural_scroll (&devicelist[i], natural_scroll);
        }

        XFreeDeviceList (devicelist);
}

static void
set_scrolling_synaptics (XDeviceInfo *device_info,
                         GSettings   *settings)
{
        touchpad_set_bool (device_info, "Synaptics Edge Scrolling", 0, g_settings_get_boolean (settings, KEY_VERT_EDGE_SCROLL));
        touchpad_set_bool (device_info, "Synaptics Edge Scrolling", 1, g_settings_get_boolean (settings, KEY_HORIZ_EDGE_SCROLL));
        touchpad_set_bool (device_info, "Synaptics Two-Finger Scrolling", 0, g_settings_get_boolean (settings, KEY_VERT_TWO_FINGER_SCROLL));
        touchpad_set_bool (device_info, "Synaptics Two-Finger Scrolling", 1, g_settings_get_boolean (settings, KEY_HORIZ_TWO_FINGER_SCROLL));
}

static void
set_scrolling_libinput (XDeviceInfo *device_info,
                        GSettings   *settings)
{
        XDevice *device;
        int format, rc;
        unsigned long nitems, bytes_after;
        unsigned char *data;
        Atom prop, type;
        gboolean want_edge, want_2fg;
        gboolean want_horiz;

        prop = property_from_name ("libinput Scroll Method Enabled");
        if (!prop) {
                return;
	}

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                return;
        }

        want_2fg = g_settings_get_boolean (settings, KEY_VERT_TWO_FINGER_SCROLL);
        want_edge = g_settings_get_boolean (settings, KEY_VERT_EDGE_SCROLL);

        /* libinput only allows for one scroll method at a time.
         * If both are set, pick 2fg scrolling.
         */
        if (want_2fg)
                want_edge = FALSE;

        g_debug ("setting scroll method on %s", device_info->name);

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 2,
                                 False, XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);

        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3) {
                data[0] = want_2fg;
                data[1] = want_edge;
                XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device,
                                       prop, XA_INTEGER, 8, PropModeReplace, data, nitems);
        }

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error in setting scroll method on \"%s\"", device_info->name);
        }

        /* Horizontal scrolling is handled by xf86-input-libinput and
         * there's only one bool. Pick the one matching the scroll method
         * we picked above.
         */
        if (want_2fg) {
                want_horiz = g_settings_get_boolean (settings, KEY_HORIZ_TWO_FINGER_SCROLL);
	}
        else if (want_edge) {
                want_horiz = g_settings_get_boolean (settings, KEY_HORIZ_EDGE_SCROLL);
	}
        else
                return;

        touchpad_set_bool (device_info, "libinput Horizontal Scroll Enabled", 0, want_horiz);
}

static void
set_scrolling (XDeviceInfo *device_info,
               GSettings   *settings)
{
        if (property_from_name ("Synaptics Edge Scrolling"))
                set_scrolling_synaptics (device_info, settings);

        if (property_from_name ("libinput Scroll Method Enabled"))
                set_scrolling_libinput (device_info, settings);
}

static void
set_scrolling_all (GSettings *settings)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        for (i = 0; i < numdevices; i++) {
                set_scrolling (&devicelist[i], settings);
        }

        XFreeDeviceList (devicelist);
}

static void
set_touchpad_enabled (XDeviceInfo *device_info,
                      gboolean     state)
{
        XDevice *device;
        Atom prop_enabled;
        unsigned char data = state;

        prop_enabled = property_from_name ("Device Enabled");
        if (!prop_enabled)
                return;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                return;
        }

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device,
                               prop_enabled, XA_INTEGER, 8,
                               PropModeReplace, &data, 1);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_flush ();
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error %s device \"%s\"",
                           (state) ? "enabling" : "disabling",
                           device_info->name);
        }
}

static void
set_touchpad_enabled_all (gboolean state)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        for (i = 0; i < numdevices; i++) {
                set_touchpad_enabled (&devicelist[i], state);
        }

        XFreeDeviceList (devicelist);
}

static void
set_disble_touchpad (XDeviceInfo *device_info, GSettings *settings)
{
        gboolean  state;
        if(strstr(device_info->name, "Mouse") != NULL && strstr(device_info->name, "USB") != NULL){
            state = g_settings_get_boolean (settings, KEY_TOUCHPAD_DISBLE_O_E_MOUSE);
            if(state){
                g_settings_set_boolean (settings, KEY_TOUCHPAD_ENABLED, FALSE);
            }else {
                g_settings_set_boolean (settings, KEY_TOUCHPAD_ENABLED, TRUE);
            }
        }
}

static void
set_plug_mouse_disble_touchpad (GSettings *settings)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        for (i = 0; i < numdevices; i++) {
                 set_disble_touchpad (&devicelist[i], settings);
        }

        XFreeDeviceList (devicelist);
}

static void
set_touchpad_double_click_drag (XDeviceInfo *device_info, gboolean state)
{
        XDevice *device;
        int format, rc;
        unsigned long nitems, bytes_after;
        unsigned char* data;
        Atom prop, type;
        Display *display = gdk_x11_get_default_xdisplay ();
        prop = property_from_name ("Synaptics Gestures");
        if (!prop)
            return;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
            return;
        }

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
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
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
                g_warning ("Error in setting double click drag  on \"%s\"", device_info->name);
        }
}

static void
set_touchpad_double_click_drag_all (gboolean state)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        for (i = 0; i < numdevices; i++) {
                 set_touchpad_double_click_drag (&devicelist[i], state);
        }

        XFreeDeviceList (devicelist);
}

static void
set_bottom_right_click_menu(UsdMouseManager *manager,
                            XDeviceInfo     *device_info,
                            gboolean state)
{
    XDevice *device;
    int format, rc;
    unsigned long nitems, bytes_after;
    unsigned char* data;
    long *ptr;
    Atom prop, type;
    Display *display = gdk_x11_get_default_xdisplay ();//QX11Info::display();
    prop = property_from_name ("Synaptics Soft Button Areas");
    if (!prop)
            return;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
        return;
    }
    gdk_x11_display_error_trap_push (gdk_display_get_default ());
    rc = XGetDeviceProperty (display , device, prop, 0, 8,
                         False, XA_INTEGER, &type, &format, &nitems,
                         &bytes_after, &data);

    if (rc == Success && type == XA_INTEGER && format == 32 && nitems >= 3) {
        ptr = (long *)data;
        if(ptr[0] != 0){
            manager->priv->mAreaLeft = ptr[0];
            manager->priv->mAreaTop  = ptr[2];
        }
        if (state) {
            ptr[0] = manager->priv->mAreaLeft;
            ptr[2] = manager->priv->mAreaTop;
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
    if (gdk_x11_display_error_trap_pop (gdk_display_get_default ())) {
        g_warning ("Error in setting bottom right click menu  on \"%s\"", device_info->name);
    }

}

static void 
set_bottom_right_conrner_click_menu(UsdMouseManager *manager, gboolean state)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        for (i = 0; i < numdevices; i++) {
                set_bottom_right_click_menu(manager, &devicelist[i], state);
        }

        XFreeDeviceList (devicelist);
}

static void
set_locate_pointer (UsdMouseManager *manager,
                    gboolean         state)
{
        if (state) {
                GError *error = NULL;
                char *args[2];

                if (manager->priv->locate_pointer_spawned)
                        return;

                args[0] = LIBEXECDIR "/usd-locate-pointer";
                args[1] = NULL;

                g_spawn_async (NULL, args, NULL,
                               0, NULL, NULL,
                               &manager->priv->locate_pointer_pid, &error);

                manager->priv->locate_pointer_spawned = (error == NULL);

                if (error) {
                        g_settings_set_boolean (manager->priv->settings_mouse, KEY_MOUSE_LOCATE_POINTER, FALSE);
                        g_error_free (error);
                }

        }
        else if (manager->priv->locate_pointer_spawned) {
                kill (manager->priv->locate_pointer_pid, SIGHUP);
                g_spawn_close_pid (manager->priv->locate_pointer_pid);
                manager->priv->locate_pointer_spawned = FALSE;
        }
}

static void
set_mouse_wheel_speed (UsdMouseManager *manager,
                       int              speed)
{
    gchar    *WheelFilePath;
    gchar    *date;
    gboolean  res;
    int delay = 240000 / speed;

    WheelFilePath = g_build_filename (g_get_home_dir (), ".imwheelrc", NULL);
    date = g_malloc (1024);
    g_sprintf(date,"\".*\"\n"\
                    "Control_L, Up,   Control_L|Button4\n"\
                    "Control_R, Up,   Control_R|Button4\n"\
                    "Control_L, Down, Control_L|Button5\n"\
                    "Control_R, Down, Control_R|Button5\n"\
                    "Shift_L,   Up,   Shift_L|Button4\n"\
                    "Shift_R,   Up,   Shift_R|Button4\n"\
                    "Shift_L,   Down, Shift_L|Button5\n"\
                    "Shift_R,   Down, Shift_R|Button5\n"\
                    "None,      Up,   Button4, %d, 0, %d\n"\
                    "None,      Down, Button5, %d, 0, %d\n",
                     speed,     delay,   speed,    delay);

    res = g_file_set_contents(WheelFilePath, date, strlen(date), NULL);
    if(!res)
        g_debug("imwheelrc File write failed ");

    GError *error = NULL;
    char *args[2];

    if (manager->priv->imwheel_spawned){
        system("killall imwheel");
        manager->priv->imwheel_spawned = FALSE;
    }
    args[0] = LIBEXECDIR "/imwheel";
    args[1] = NULL;

    g_spawn_async (NULL, args, NULL,
                   0, NULL, NULL,
                   NULL, &error);
    manager->priv->imwheel_spawned = (error == NULL);
    g_free(date);

}
#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
static void
set_mousetweaks_daemon (UsdMouseManager *manager,
                        gboolean         dwell_enable,
                        gboolean         delay_enable)
{
        GError *error = NULL;
        gchar *comm;
        gboolean run_daemon = dwell_enable || delay_enable;

        if (run_daemon || manager->priv->mousetweaks_daemon_running)
                comm = g_strdup_printf ("mousetweaks %s",
                                        run_daemon ? "" : "-s");
        else
                return;

        if (run_daemon)
                manager->priv->mousetweaks_daemon_running = TRUE;


        if (! g_spawn_command_line_async (comm, &error)) {
                if (error->code == G_SPAWN_ERROR_NOENT &&
                    (dwell_enable || delay_enable)) {
                        GtkWidget *dialog;

                        GSettings *settings;
                        settings = g_settings_new (UKUI_MOUSE_A11Y_SCHEMA);
                        if (dwell_enable)
                                g_settings_set_boolean (settings,
                                                        UKUI_MOUSE_A11Y_KEY_DWELL_ENABLE,
                                                        FALSE);
                        else if (delay_enable)
                                g_settings_set_boolean (settings,
                                                        UKUI_MOUSE_A11Y_KEY_DELAY_ENABLE,
                                                        FALSE);
                        g_object_unref (settings);

                        dialog = gtk_message_dialog_new (NULL, 0,
                                                         GTK_MESSAGE_WARNING,
                                                         GTK_BUTTONS_OK,
                                                         _("Could not enable mouse accessibility features"));
                        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                                                  _("Mouse accessibility requires Mousetweaks "
                                                                    "to be installed on your system."));
                        gtk_window_set_title (GTK_WINDOW (dialog),
                                              _("Mouse Preferences"));
                        gtk_window_set_icon_name (GTK_WINDOW (dialog),
                                                  "input-mouse");
                        gtk_dialog_run (GTK_DIALOG (dialog));
                        gtk_widget_destroy (dialog);
                }
                g_error_free (error);
        }
        g_free (comm);
}
#endif  /* set_mousetweaks_daemon */

static void
set_mouse_settings (UsdMouseManager *manager)
{
        gboolean mouse_left_handed = g_settings_get_boolean (manager->priv->settings_mouse, KEY_LEFT_HANDED);
        gboolean touchpad_left_handed = get_touchpad_handedness (manager, mouse_left_handed);
        set_left_handed_all (manager, mouse_left_handed, touchpad_left_handed);

        set_motion_all (manager);
        set_mouse_wheel_speed (manager, g_settings_get_int (smanager->priv->settings_mous, KEY_MOUSE_WHEEL_SPEED));
        //set_middle_button_all (g_settings_get_boolean (manager->priv->settings_mouse, KEY_MIDDLE_BUTTON_EMULATION));
}

static void
set_touchpad_settings (UsdMouseManager *manager)
{
    set_disable_w_typing (manager, g_settings_get_boolean (manager->priv->settings_touchpad, KEY_TOUCHPAD_DISABLE_W_TYPING));

    set_tap_to_click_all (manager);

    set_scrolling_all (manager->priv->settings_touchpad);
    set_natural_scroll_all (manager);
    set_touchpad_enabled_all (g_settings_get_boolean (manager->priv->settings_touchpad, KEY_TOUCHPAD_ENABLED));

    if (g_settings_get_boolean (manager->priv->settings_touchpad, KEY_TOUCHPAD_DISBLE_O_E_MOUSE))
          set_plug_mouse_disble_touchpad (manager->priv->settings_touchpad);

    set_touchpad_double_click_drag_all (g_settings_get_boolean (manager->priv->settings_touchpad,
                                                                    KEY_TOUCHPAD_DOUBLE_CLICK_DRAG));
    set_bottom_right_conrner_click_menu (manager, g_settings_get_boolean (manager->priv->settings_touchpad,
                                                                              KEY_TOUCHPAD_BOTTOM_R_C_CLICK_M));
}

static void
mouse_callback (GSettings          *settings,
                const gchar        *key,
                UsdMouseManager    *manager)
{
        if (g_strcmp0 (key, KEY_LEFT_HANDED) == 0) {
                gboolean mouse_left_handed = g_settings_get_boolean (settings, key);
                gboolean touchpad_left_handed = get_touchpad_handedness (manager, mouse_left_handed);
                set_left_handed_all (manager, mouse_left_handed, touchpad_left_handed);
        } else if ((g_strcmp0 (key, KEY_MOTION_ACCELERATION) == 0)
                || (g_strcmp0 (key, KEY_MOTION_THRESHOLD) == 0)
                || (g_strcmp0 (key, KEY_MOUSE_ACCEL) == 0)) {
                set_motion_all (manager);
        } else if (g_strcmp0 (key, KEY_MIDDLE_BUTTON_EMULATION) == 0) {
                set_middle_button_all (g_settings_get_boolean (settings, key));
        } else if (g_strcmp0 (key, KEY_MOUSE_LOCATE_POINTER) == 0) {
                set_locate_pointer (manager, g_settings_get_boolean (settings, key));
        } else if (g_strcmp0 (key, KEY_MOUSE_WHEEL_SPEED) == 0 ) {
                set_mouse_wheel_speed (manager, g_settings_get_int (settings, key));
#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
        } else if (g_strcmp0 (key, KEY_MOUSE_A11Y_DWELL_ENABLE) == 0) {
                set_mousetweaks_daemon (manager,
                                        g_settings_get_boolean (settings, key),
                                        g_settings_get_boolean (settings, KEY_MOUSE_A11Y_DELAY_ENABLE, NULL));
                }
        } else if (g_strcmp0 (key, KEY_MOUSE_A11Y_DELAY_ENABLE) == 0) {
                set_mousetweaks_daemon (manager,
                                        g_settings_get_boolean (settings, KEY_MOUSE_A11Y_DWELL_ENABLE, NULL),
                                        g_settings_get_boolean (settings, key));
#endif
        }
}

static void
touchpad_callback (GSettings          *settings,
                   const gchar        *key,
                   UsdMouseManager    *manager)
{
        if (g_strcmp0 (key, KEY_TOUCHPAD_DISABLE_W_TYPING) == 0) {
                set_disable_w_typing (manager, g_settings_get_boolean (settings, key));
        } else if (g_strcmp0 (key, KEY_LEFT_HANDED) == 0) {
                gboolean mouse_left_handed = g_settings_get_boolean (manager->priv->settings_mouse, key);
                gboolean touchpad_left_handed = get_touchpad_handedness (manager, mouse_left_handed);
                set_left_handed_all (manager, mouse_left_handed, touchpad_left_handed);
        } else if ((g_strcmp0 (key, KEY_TOUCHPAD_TAP_TO_CLICK) == 0)
                || (g_strcmp0 (key, KEY_TOUCHPAD_ONE_FINGER_TAP) == 0)
                || (g_strcmp0 (key, KEY_TOUCHPAD_TWO_FINGER_TAP) == 0)
                || (g_strcmp0 (key, KEY_TOUCHPAD_THREE_FINGER_TAP) == 0)) {
                set_tap_to_click_all (manager);
/* Do not set click actions since ukwm take over this
 *        } else if ((g_strcmp0 (key, KEY_TOUCHPAD_TWO_FINGER_CLICK) == 0)
 *               || (g_strcmp0 (key, KEY_TOUCHPAD_THREE_FINGER_CLICK) == 0)) {
 *               set_click_actions_all (manager);
 */
        } else if ((g_strcmp0 (key, KEY_VERT_EDGE_SCROLL) == 0)
                || (g_strcmp0 (key, KEY_HORIZ_EDGE_SCROLL) == 0)
                || (g_strcmp0 (key, KEY_VERT_TWO_FINGER_SCROLL) == 0)
                || (g_strcmp0 (key, KEY_HORIZ_TWO_FINGER_SCROLL) == 0)) {
                set_scrolling_all (manager->priv->settings_touchpad);
        } else if (g_strcmp0 (key, KEY_TOUCHPAD_NATURAL_SCROLL) == 0) {
                set_natural_scroll_all (manager);
        } else if (g_strcmp0 (key, KEY_TOUCHPAD_ENABLED) == 0) {
                set_touchpad_enabled_all (g_settings_get_boolean (settings, key));
        } else if ((g_strcmp0 (key, KEY_MOTION_ACCELERATION) == 0)
                || (g_strcmp0 (key, KEY_MOTION_THRESHOLD) == 0)) {
                set_motion_all (manager);
        } else if (g_strcmp0 (key, KEY_TOUCHPAD_DISBLE_O_E_MOUSE) == 0) {
                set_plug_mouse_disble_touchpad (manager->priv->settings_touchpad);//设置插入鼠标时禁用触摸板
        } else if (g_strcmp0 (key, KEY_TOUCHPAD_DOUBLE_CLICK_DRAG) == 0) {
                set_touchpad_double_click_drag_all (g_settings_get_boolean (settings, key));//设置轻点击两次拖动打开关闭
        }else if (g_strcmp0 (key, KEY_TOUCHPAD_BOTTOM_R_C_CLICK_M) == 0) {
                set_bottom_right_conrner_click_menu (manager, g_settings_get_boolean (settings, key));//打开关闭右下角点击弹出菜单
        }
}

static void
usd_mouse_manager_init (UsdMouseManager *manager)
{
        manager->priv = USD_MOUSE_MANAGER_GET_PRIVATE (manager);
}

static gboolean
usd_mouse_manager_idle_cb (UsdMouseManager *manager)
{
        ukui_settings_profile_start (NULL);
        manager->priv->mAreaLeft = 0;
        manager->priv->mAreaTop  = 0;
        manager->priv->settings_mouse = g_settings_new (UKUI_MOUSE_SCHEMA);
        manager->priv->settings_touchpad = g_settings_new (UKUI_TOUCHPAD_SCHEMA);

        g_signal_connect (manager->priv->settings_mouse, "changed",
                          G_CALLBACK (mouse_callback), manager);
        g_signal_connect (manager->priv->settings_touchpad, "changed",
                          G_CALLBACK (touchpad_callback), manager);

        manager->priv->syndaemon_spawned = FALSE;

        set_devicepresence_handler (manager);

        set_mouse_settings (manager);
        set_locate_pointer (manager, g_settings_get_boolean (manager->priv->settings_mouse, KEY_MOUSE_LOCATE_POINTER));
        set_touchpad_settings (manager);

#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
        set_mousetweaks_daemon (manager,
                                g_settings_get_boolean (manager->priv->settings_mouse_a11y,
                                                        KEY_MOUSE_A11Y_DWELL_ENABLE),
                                g_settings_get_boolean (manager->priv->settings_mouse_a11y,
                                                        KEY_MOUSE_A11Y_DELAY_ENABLE));
#endif

        ukui_settings_profile_end (NULL);

        return FALSE;
}

gboolean
usd_mouse_manager_start (UsdMouseManager *manager,
                         GError         **error)
{
        ukui_settings_profile_start (NULL);

        if (!supports_xinput_devices ()) {
                g_debug ("XInput is not supported, not applying any settings");
                return TRUE;
        }

        g_idle_add ((GSourceFunc) usd_mouse_manager_idle_cb, manager);

        ukui_settings_profile_end (NULL);

        return TRUE;
}

void
usd_mouse_manager_stop (UsdMouseManager *manager)
{
        UsdMouseManagerPrivate *p = manager->priv;

        g_debug ("Stopping mouse manager");

        if (p->settings_mouse != NULL) {
                g_object_unref(p->settings_mouse);
                p->settings_mouse = NULL;
        }

        if (p->settings_touchpad != NULL) {
                g_object_unref(p->settings_touchpad);
                p->settings_touchpad = NULL;
        }

        set_locate_pointer (manager, FALSE);

        gdk_window_remove_filter (NULL, devicepresence_filter, manager);
}

static void
usd_mouse_manager_finalize (GObject *object)
{
        UsdMouseManager *mouse_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (USD_IS_MOUSE_MANAGER (object));

        mouse_manager = USD_MOUSE_MANAGER (object);

        g_return_if_fail (mouse_manager->priv != NULL);

        G_OBJECT_CLASS (usd_mouse_manager_parent_class)->finalize (object);
}

UsdMouseManager *
usd_mouse_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (USD_TYPE_MOUSE_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return USD_MOUSE_MANAGER (manager_object);
}
