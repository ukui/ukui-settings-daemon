#include "mouse-manager.h"
#include "clib-syslog.h"
#include "../common/usd-input-helper.h"

typedef enum {
        TOUCHPAD_HANDEDNESS_RIGHT,
        TOUCHPAD_HANDEDNESS_LEFT,
        TOUCHPAD_HANDEDNESS_MOUSE
} TouchpadHandedness;


gboolean usd_mouse_manager_idle_cb(MouseManager *manager);
GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
                                       GdkEvent  *event,
                                       gpointer   data);

MouseManager * MouseManager::mMouseManager =nullptr;

MouseManager::MouseManager(QObject *parent) : QObject (parent)
{
}

MouseManager * MouseManager::MouseManagerNew()
{
    if(nullptr == mMouseManager)
        mMouseManager = new MouseManager(nullptr);

    return mMouseManager;
}

bool MouseManager::MouseManagerStart(GError** error)
{
    CT_SYSLOG(LOG_DEBUG,"-- Mouse Start Manager --");
    /* --- wait common ---
     * if (!supports_xinput_devices()){
        CT_SYSLOG(LOG_ERR,"XInput is not supported, not applying any settings");
        return TRUE;
    }*/
    g_idle_add((GSourceFunc) usd_mouse_manager_idle_cb,this);

    return true;
}

void MouseManager::MouseManagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"-- Stoping Mouse Manager --");
    if (this->settings_mouse != NULL) {
            g_object_unref(this->settings_mouse);
            this->settings_mouse = NULL;
    }

    if (this->settings_touchpad != NULL) {
            g_object_unref(this->settings_touchpad);
            this->settings_touchpad = NULL;
    }

    set_locate_pointer (this, FALSE);

    gdk_window_remove_filter (NULL, devicepresence_filter, this);
}

gboolean get_touchpad_handedness (MouseManager *manager,
                                  gboolean  mouse_left_handed)
{
        switch (g_settings_get_enum (manager->settings_touchpad, KEY_LEFT_HANDED)) {
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
Atom property_from_name (const char *property_name)
{
       return XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), property_name, True);
}

gboolean property_exists_on_device (XDeviceInfo *device_info, const char  *property_name)
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

        gdk_error_trap_push ();
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_error_trap_pop () != 0) || (device == NULL))
                return FALSE;

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                 device, prop, 0, 1, False, XA_INTEGER, &type, &format,
                                 &nitems, &bytes_after, &data);

        if (rc == Success)
                XFree (data);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_error_trap_pop_ignored ();

        return rc == Success;
}

void property_set_bool (XDeviceInfo *device_info,
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

        gdk_error_trap_push ();
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

        if (gdk_error_trap_pop ()) {
                g_warning ("Error while setting %s on \"%s\"", property_name, device_info->name);
        }
}

void set_left_handed_libinput (XDeviceInfo *device_info,
                          gboolean     mouse_left_handed,
                          gboolean     touchpad_left_handed)
{
        XDevice *device;
        gboolean want_lefthanded;

        device = device_is_touchpad (device_info);
        if (device == NULL) {
                gdk_error_trap_push ();
                device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
                if ((gdk_error_trap_pop () != 0) || (device == NULL))
                        return;

                want_lefthanded = mouse_left_handed;
        } else {
                /* touchpad device is already open after
                 * return from device_is_touchpad function
                 */
                want_lefthanded = touchpad_left_handed;
        }

        property_set_bool (device_info, device, "libinput Left Handed Enabled", 0, want_lefthanded);

        gdk_error_trap_push ();
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_error_trap_pop_ignored ();
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

gboolean touchpad_has_single_button (XDevice *device)
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

        gdk_error_trap_push ();
        rc = XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 1, False,
                                 XA_INTEGER, &type, &format, &nitems,
                                 &bytes_after, &data);
        if (rc == Success && type == XA_INTEGER && format == 8 && nitems >= 3)
                is_single_button = (data[0] == 1 && data[1] == 0 && data[2] == 0);

        if (rc == Success)
                XFree (data);

        gdk_error_trap_pop_ignored ();

        return is_single_button;
}

void set_tap_to_click_synaptics (XDeviceInfo *device_info,
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

        gdk_error_trap_push ();
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
        if (gdk_error_trap_pop ()) {
                g_warning ("Error in setting tap to click on \"%s\"", device_info->name);
        }
}

void configure_button_layout (guchar   *buttons,
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

void set_left_handed_legacy_driver (MouseManager *manager,
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
                gboolean tap = g_settings_get_boolean (manager->settings_touchpad, KEY_TOUCHPAD_TAP_TO_CLICK);
                gboolean single_button = touchpad_has_single_button (device);

                left_handed = touchpad_left_handed;

                if (tap && !single_button) {
                        gint one_finger_tap = g_settings_get_int (manager->settings_touchpad, KEY_TOUCHPAD_ONE_FINGER_TAP);
                        gint two_finger_tap = g_settings_get_int (manager->settings_touchpad, KEY_TOUCHPAD_TWO_FINGER_TAP);
                        gint three_finger_tap = g_settings_get_int (manager->settings_touchpad, KEY_TOUCHPAD_THREE_FINGER_TAP);
                        set_tap_to_click_synaptics (device_info, tap, left_handed, one_finger_tap, two_finger_tap, three_finger_tap);
                }

                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);

                if (single_button)
                        return;
        } else {
                left_handed = mouse_left_handed;
        }

        gdk_error_trap_push ();

        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_error_trap_pop () != 0) ||
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

void set_left_handed (MouseManager *manager,
                 XDeviceInfo     *device_info,
                 gboolean         mouse_left_handed,
                 gboolean         touchpad_left_handed)
{
        if (property_exists_on_device (device_info, "libinput Left Handed Enabled"))
                set_left_handed_libinput (device_info, mouse_left_handed, touchpad_left_handed);
        else
                set_left_handed_legacy_driver (manager, device_info, mouse_left_handed, touchpad_left_handed);
}

void set_left_handed_all (MouseManager *manager,
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

void set_motion_libinput (MouseManager *manager,
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
                settings = manager->settings_touchpad;
        } else {
                gdk_error_trap_push ();
                device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
                if ((gdk_error_trap_pop () != 0) || (device == NULL))
                        return;

                settings = manager->settings_mouse;
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

        gdk_error_trap_push ();
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
        if (gdk_error_trap_pop ()) {
                g_warning ("Error while setting accel speed on \"%s\"", device_info->name);
        }
}

void set_motion_legacy_driver (MouseManager *manager,
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
                settings = manager->settings_touchpad;
        } else {
                gdk_error_trap_push ();
                device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
                if ((gdk_error_trap_pop () != 0) || (device == NULL))
                        return;

                settings = manager->settings_mouse;
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
                if (state->c_class == PtrFeedbackClass) {
                        /* And tell the device */
                        feedback.c_class      = PtrFeedbackClass;
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

void set_motion (MouseManager *manager,
                 XDeviceInfo     *device_info)
{
        if (property_exists_on_device (device_info, "libinput Accel Speed"))
                set_motion_libinput (manager, device_info);
        else
                set_motion_legacy_driver (manager, device_info);
}

void set_motion_all (MouseManager *manager)
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

void set_middle_button_evdev (XDeviceInfo *device_info,
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

        gdk_error_trap_push ();

        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_error_trap_pop () != 0) ||
            (device == NULL))
                return;

        gdk_error_trap_push ();
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
        if (gdk_error_trap_pop ()) {
                g_warning ("Error in setting middle button emulation on \"%s\"", device_info->name);
        }
}

void set_middle_button_libinput (XDeviceInfo *device_info,
                                 gboolean     middle_button)
{
        XDevice *device;

        /* touchpad devices are excluded as the old code
         * only applies to evdev devices
         */
        device = device_is_touchpad (device_info);
        if (device != NULL) {
                gdk_error_trap_push ();
                XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
                gdk_error_trap_pop_ignored ();
                return;
        }

        gdk_error_trap_push ();
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device_info->id);
        if ((gdk_error_trap_pop () != 0) || (device == NULL))
                return;

        property_set_bool (device_info, device, "libinput Middle Emulation Enabled", 0, middle_button);

        gdk_error_trap_push ();
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_error_trap_pop_ignored ();
}

void set_middle_button (XDeviceInfo *device_info,
                        gboolean     middle_button)
{
        if (property_from_name ("Evdev Middle Button Emulation"))
                set_middle_button_evdev (device_info, middle_button);

        if (property_from_name ("libinput Middle Emulation Enabled"))
                set_middle_button_libinput (device_info, middle_button);
}

void set_middle_button_all (gboolean middle_button)
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


void set_locate_pointer (MouseManager *manager, gboolean     state)
{
        if (state) {
                GError *error = NULL;
                char *args[2];

                if (manager->locate_pointer_spawned)
                        return;

                args[0] ="/usr/bin/usd-locate-pointer";
                args[1] = NULL;

                g_spawn_async (NULL, args, NULL,
                              (GSpawnFlags)0, NULL, NULL,
                               &manager->locate_pointer_pid, &error);

                manager->locate_pointer_spawned = (error == NULL);

                if (error) {
                        g_settings_set_boolean (manager->settings_mouse, KEY_MOUSE_LOCATE_POINTER, FALSE);
                        g_error_free (error);
                }

        }
        else if (manager->locate_pointer_spawned) {
                kill (manager->locate_pointer_pid, SIGHUP);
                g_spawn_close_pid (manager->locate_pointer_pid);
                manager->locate_pointer_spawned = FALSE;
        }
}

void mouse_callback (GSettings          *settings,
                    const gchar        *key,
                    MouseManager    *manager)
{
        if (g_strcmp0 (key, KEY_LEFT_HANDED) == 0) {
                gboolean mouse_left_handed = g_settings_get_boolean (settings, key);
                gboolean touchpad_left_handed = get_touchpad_handedness (manager, mouse_left_handed);
                set_left_handed_all (manager, mouse_left_handed, touchpad_left_handed);
        } else if ((g_strcmp0 (key, KEY_MOTION_ACCELERATION) == 0)
                || (g_strcmp0 (key, KEY_MOTION_THRESHOLD) == 0)) {
                set_motion_all (manager);
        } else if (g_strcmp0 (key, KEY_MIDDLE_BUTTON_EMULATION) == 0) {
                set_middle_button_all (g_settings_get_boolean (settings, key));
        } else if (g_strcmp0 (key, KEY_MOUSE_LOCATE_POINTER) == 0) {
                set_locate_pointer (manager, g_settings_get_boolean (settings, key));

        }
}

gboolean have_program_in_path (const char *name)
{
        gchar *path;
        gboolean result;

        path = g_find_program_in_path (name);
        result = (path != NULL);
        g_free (path);
        return result;
}
void set_disable_w_typing_synaptics (MouseManager *manager,
                                    gboolean         state)
{
        if (state && touchpad_is_present ()) {
                GError *error = NULL;
                char *args[6];

                if (manager->syndaemon_spawned)
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
                                &manager->syndaemon_pid, &error);

                manager->syndaemon_spawned = (error == NULL);

                if (error) {
                        g_settings_set_boolean (manager->settings_touchpad, KEY_TOUCHPAD_DISABLE_W_TYPING, FALSE);
                        g_error_free (error);
                }

        } else if (manager->syndaemon_spawned)
        {
                kill (manager->syndaemon_pid, SIGHUP);
                g_spawn_close_pid (manager->syndaemon_pid);
                manager->syndaemon_spawned = FALSE;
        }
}
void touchpad_set_bool (XDeviceInfo *device_info,
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

        gdk_error_trap_push ();
        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_error_trap_pop_ignored ();
}

void set_disable_w_typing_libinput (MouseManager *manager,
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

void set_disable_w_typing (MouseManager *manager,
                           gboolean         state)
{
        if (property_from_name ("Synaptics Off"))
                set_disable_w_typing_synaptics (manager, state);

        if (property_from_name ("libinput Disable While Typing Enabled"))
                set_disable_w_typing_libinput (manager, state);
}

void set_tap_to_click_all (MouseManager *manager)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        gboolean state = g_settings_get_boolean (manager->settings_touchpad, KEY_TOUCHPAD_TAP_TO_CLICK);
        gboolean left_handed = get_touchpad_handedness (manager, g_settings_get_boolean (manager->settings_mouse, KEY_LEFT_HANDED));
        gint one_finger_tap = g_settings_get_int (manager->settings_touchpad, KEY_TOUCHPAD_ONE_FINGER_TAP);
        gint two_finger_tap = g_settings_get_int (manager->settings_touchpad, KEY_TOUCHPAD_TWO_FINGER_TAP);
        gint three_finger_tap = g_settings_get_int (manager->settings_touchpad, KEY_TOUCHPAD_THREE_FINGER_TAP);

        for (i = 0; i < numdevices; i++) {
//                set_tap_to_click (&devicelist[i], state, left_handed, one_finger_tap, two_finger_tap, three_finger_tap);
        }

        XFreeDeviceList (devicelist);
}

static void set_scrolling_synaptics (XDeviceInfo *device_info,
                                     GSettings   *settings)
{
        touchpad_set_bool (device_info, "Synaptics Edge Scrolling", 0, g_settings_get_boolean (settings, KEY_VERT_EDGE_SCROLL));
        touchpad_set_bool (device_info, "Synaptics Edge Scrolling", 1, g_settings_get_boolean (settings, KEY_HORIZ_EDGE_SCROLL));
        touchpad_set_bool (device_info, "Synaptics Two-Finger Scrolling", 0, g_settings_get_boolean (settings, KEY_VERT_TWO_FINGER_SCROLL));
        touchpad_set_bool (device_info, "Synaptics Two-Finger Scrolling", 1, g_settings_get_boolean (settings, KEY_HORIZ_TWO_FINGER_SCROLL));
}


static void set_scrolling_libinput (XDeviceInfo *device_info,
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
        if (!prop)
                return;

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

        gdk_error_trap_push ();
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
        if (gdk_error_trap_pop ()) {
                g_warning ("Error in setting scroll method on \"%s\"", device_info->name);
        }

        /* Horizontal scrolling is handled by xf86-input-libinput and
         * there's only one bool. Pick the one matching the scroll method
         * we picked above.
         */
        if (want_2fg)
                want_horiz = g_settings_get_boolean (settings, KEY_HORIZ_TWO_FINGER_SCROLL);
        else if (want_edge)
                want_horiz = g_settings_get_boolean (settings, KEY_HORIZ_EDGE_SCROLL);
        else
                return;

        touchpad_set_bool (device_info, "libinput Horizontal Scroll Enabled", 0, want_horiz);
}

static void set_scrolling (XDeviceInfo *device_info,
                           GSettings   *settings)
 {
         if (property_from_name ("Synaptics Edge Scrolling"))
                 set_scrolling_synaptics (device_info, settings);

         if (property_from_name ("libinput Scroll Method Enabled"))
                 set_scrolling_libinput (device_info, settings);
 }

void set_scrolling_all (GSettings *settings)
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

void set_natural_scroll_synaptics (XDeviceInfo *device_info,
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

        gdk_error_trap_push ();
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
        if (gdk_error_trap_pop ()) {
                g_warning ("Error in setting natural scroll on \"%s\"", device_info->name);
        }
}

void set_natural_scroll_libinput (XDeviceInfo *device_info,
                                  gboolean     natural_scroll)
{
        g_debug ("Trying to set %s for \"%s\"", natural_scroll ? "natural (reverse) scroll" : "normal scroll", device_info->name);

        touchpad_set_bool (device_info, "libinput Natural Scrolling Enabled", 0, natural_scroll);
}


void set_natural_scroll (XDeviceInfo *device_info,
                    gboolean     natural_scroll)
{
        if (property_from_name ("Synaptics Scrolling Distance"))
                set_natural_scroll_synaptics (device_info, natural_scroll);

        if (property_from_name ("libinput Natural Scrolling Enabled"))
                set_natural_scroll_libinput (device_info, natural_scroll);
}

void set_natural_scroll_all (MouseManager *manager)
{
        int numdevices, i;
        XDeviceInfo *devicelist = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &numdevices);

        if (devicelist == NULL)
                return;

        gboolean natural_scroll = g_settings_get_boolean (manager->settings_touchpad, KEY_TOUCHPAD_NATURAL_SCROLL);

        for (i = 0; i < numdevices; i++) {
                set_natural_scroll (&devicelist[i], natural_scroll);
        }

        XFreeDeviceList (devicelist);
}

void set_touchpad_enabled (XDeviceInfo *device_info,
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

        gdk_error_trap_push ();
        XChangeDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device,
                               prop_enabled, XA_INTEGER, 8,
                               PropModeReplace, &data, 1);

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        gdk_flush ();
        if (gdk_error_trap_pop ()) {
                g_warning ("Error %s device \"%s\"",
                           (state) ? "enabling" : "disabling",
                           device_info->name);
        }
}

void set_touchpad_enabled_all (gboolean state)
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

void touchpad_callback (GSettings          *settings,
                       const gchar        *key,
                       MouseManager    *manager)
{
        if (g_strcmp0 (key, KEY_TOUCHPAD_DISABLE_W_TYPING) == 0) {
                set_disable_w_typing (manager, g_settings_get_boolean (settings, key));
        } else if (g_strcmp0 (key, KEY_LEFT_HANDED) == 0) {
                gboolean mouse_left_handed = g_settings_get_boolean (manager->settings_mouse, key);
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
                set_scrolling_all (manager->settings_touchpad);
        } else if (g_strcmp0 (key, KEY_TOUCHPAD_NATURAL_SCROLL) == 0) {
                set_natural_scroll_all (manager);
        } else if (g_strcmp0 (key, KEY_TOUCHPAD_ENABLED) == 0) {
                set_touchpad_enabled_all (g_settings_get_boolean (settings, key));
        } else if ((g_strcmp0 (key, KEY_MOTION_ACCELERATION) == 0)
                || (g_strcmp0 (key, KEY_MOTION_THRESHOLD) == 0)) {
                set_motion_all (manager);
        }
}

void set_mouse_settings (MouseManager *manager)
{
        gboolean mouse_left_handed = g_settings_get_boolean (manager->settings_mouse, KEY_LEFT_HANDED);
        gboolean touchpad_left_handed = get_touchpad_handedness (manager, mouse_left_handed);
        set_left_handed_all (manager, mouse_left_handed, touchpad_left_handed);

        set_motion_all (manager);
        set_middle_button_all (g_settings_get_boolean (manager->settings_mouse, KEY_MIDDLE_BUTTON_EMULATION));

        set_disable_w_typing (manager, g_settings_get_boolean (manager->settings_touchpad, KEY_TOUCHPAD_DISABLE_W_TYPING));

        set_tap_to_click_all (manager);
/* Do not set click actions since ukwm take over this
 *
 *       set_click_actions_all (manager);
 */
        set_scrolling_all (manager->settings_touchpad);
        set_natural_scroll_all (manager);
        set_touchpad_enabled_all (g_settings_get_boolean (manager->settings_touchpad, KEY_TOUCHPAD_ENABLED));
}

GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
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
                        set_mouse_settings ((MouseManager *) data);
        }

        return GDK_FILTER_CONTINUE;
}

void set_devicepresence_handler (MouseManager *manager)
{
        Display *display;
        XEventClass class_presence;
        int xi_presence;

        display = gdk_x11_get_default_xdisplay ();

        gdk_error_trap_push ();
        DevicePresence (display, xi_presence, class_presence);
        XSelectExtensionEvent (display,
                               RootWindow (display, DefaultScreen (display)),
                               &class_presence, 1);

        gdk_flush ();
        if (!gdk_error_trap_pop ())
                gdk_window_add_filter (NULL, devicepresence_filter, manager);
}

gboolean usd_mouse_manager_idle_cb(MouseManager *manager)
{

    manager->settings_mouse = g_settings_new(UKUI_MOUSE_SCHEMA);
    manager->settings_touchpad = g_settings_new(UKUI_TOUCHPAD_SCHEMA);

    g_signal_connect (manager->settings_mouse, "changed",
                              G_CALLBACK (mouse_callback), manager);
    g_signal_connect (manager->settings_touchpad, "changed",
                      G_CALLBACK (touchpad_callback), manager);

    manager->syndaemon_spawned = FALSE;

    set_devicepresence_handler (manager);

    set_mouse_settings (manager);
    set_locate_pointer (manager, g_settings_get_boolean (manager->settings_mouse,
                                                         KEY_MOUSE_LOCATE_POINTER));
}


