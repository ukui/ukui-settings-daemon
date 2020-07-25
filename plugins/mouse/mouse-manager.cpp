#include "mouse-manager.h"
#include "clib-syslog.h"

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
    settings_mouse =    new QGSettings(UKUI_MOUSE_SCHEMA);
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
    CT_SYSLOG(LOG_DEBUG,"-- Mouse Start Manager --");

    if (!supports_xinput_devices()){
        CT_SYSLOG(LOG_ERR,"XInput is not supported, not applying any settings");
        return TRUE;
    }
    time = new QTimer(this);
    connect(time,SIGNAL(timeout()),this,SLOT(usd_mouse_manager_idle_cb()));
    time->start();
    return true;
}

void MouseManager::MouseManagerStop()
{

    syslog(LOG_DEBUG,"-- Stoping Mouse Manager --");

    set_locate_pointer (this, FALSE);

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



bool MouseManager::get_touchpad_handedness (bool  mouse_left_handed)
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

gboolean property_exists_on_device (XDeviceInfo *device_info, const char  *property_name)
{
    XDevice *device;
    int rc;
    Atom type, prop;
    int format;
    unsigned long nitems, bytes_after;
    unsigned char *data;
    Display *display = QX11Info::display();
    prop = property_from_name (property_name);
    if (!prop)
            return FALSE;
    try {
        device = XOpenDevice (display, device_info->id);
        if (device == NULL)
            throw 1;
        rc = XGetDeviceProperty (display,
                                 device, prop, 0, 1, False, XA_INTEGER, &type, &format,
                                 &nitems, &bytes_after, &data);
        if (rc == Success)
                XFree (data);

        XCloseDevice (display, device);
    } catch (int str) {
        CT_SYSLOG(LOG_DEBUG,"MOUSE: WRING ID: %d",str);
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
        if (rc == Success && act_type == XA_INTEGER && act_format == 8 && nitems > property_index) {
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
        CT_SYSLOG(LOG_DEBUG,"MOUSE:Error while setting %s on \"%s\"", property_name, device_info->name)

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
        if (three_finger_tap > 3 || three_finger_tap < 1)
                three_finger_tap = 2;

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
        CT_SYSLOG(LOG_ERR,"Error in setting tap to click on \"%s\"", device_info->name);
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

void set_left_handed_legacy_driver (MouseManager *manager,
                                    XDeviceInfo     *device_info,
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
            bool tap = manager->settings_touchpad->get(KEY_TOUCHPAD_TAP_TO_CLICK).toBool();
            bool single_button = touchpad_has_single_button (device);

            left_handed = touchpad_left_handed;

            if (tap && !single_button) {
                    int one_finger_tap = manager->settings_touchpad->get(KEY_TOUCHPAD_ONE_FINGER_TAP).toInt();
                    int two_finger_tap = manager->settings_touchpad->get(KEY_TOUCHPAD_TWO_FINGER_TAP).toInt();
                    int three_finger_tap = manager->settings_touchpad->get(KEY_TOUCHPAD_THREE_FINGER_TAP).toInt();
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

        while (n_buttons > buttons_capacity) {
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
        CT_SYSLOG(LOG_DEBUG,"MOUSE :error id %d",x);
        return;
    }
}

void set_left_handed (MouseManager *manager,
                      XDeviceInfo  *device_info,
                      bool         mouse_left_handed,
                      bool         touchpad_left_handed)
{
    if (property_exists_on_device (device_info, "libinput Left Handed Enabled"))
        set_left_handed_libinput (device_info, mouse_left_handed, touchpad_left_handed);
    else
        set_left_handed_legacy_driver (manager, device_info, mouse_left_handed, touchpad_left_handed);
}

void set_left_handed_all (MouseManager *manager,
                         bool         mouse_left_handed,
                         bool        touchpad_left_handed)
{
    XDeviceInfo *device_info;
    int n_devices;
    int i;
    Display * dpy = QX11Info::display();
    device_info = XListInputDevices (dpy, &n_devices);
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
    QGSettings *settings;

    Display * dpy = gdk_x11_get_default_xdisplay ();//QX11Info::display();

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
            settings = manager->settings_touchpad;
        } else {
            device = XOpenDevice (dpy, device_info->id);
            if (device == NULL)
                throw 1;
            settings = manager->settings_mouse;
        }
        /* Calculate acceleration */
        motion_acceleration = settings->get(KEY_MOTION_ACCELERATION).Double;

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
        CT_SYSLOG(LOG_ERR,"%s Error while setting accel speed on \"%s\"", device_info->name);
        return;
    }
}

void set_motion_legacy_driver (MouseManager *manager,
                               XDeviceInfo     *device_info)
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
            settings = manager->settings_touchpad;
    } else {
        try {
            device = XOpenDevice (dpy, device_info->id);
            if (device == NULL)
                throw 1;
        } catch (int x) {
            CT_SYSLOG(LOG_DEBUG,"%s: error id %d","MOUSE", x);
            return;
        }
            settings = manager->settings_mouse;
    }

    /* Calculate acceleration */
    motion_acceleration = settings->get(KEY_MOTION_ACCELERATION).Double;

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

            qDebug ("Setting accel %d/%d, threshold %d for device '%s'",
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
    int n_devices;
    int i;

    device_info = XListInputDevices (QX11Info::display(), &n_devices);
    for (i = 0; i < n_devices; i++) {
        set_motion (manager, &device_info[i]);
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

    Display * display = QX11Info::display();
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
        CT_SYSLOG(LOG_ERR,"Error in setting middle button emulation on \"%s\"", device_info->name);
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
        CT_SYSLOG(LOG_DEBUG,"%s:error id %d","MOUSE",x);
        return;
    }
}

void set_middle_button (XDeviceInfo *device_info,
                        bool     middle_button)
{
    if (property_from_name ("Evdev Middle Button Emulation"))
        set_middle_button_evdev (device_info, middle_button);

    if (property_from_name ("libinput Middle Emulation Enabled"))
        set_middle_button_libinput (device_info, middle_button);
}

void set_middle_button_all (bool middle_button)
{
    XDeviceInfo *device_info;
    int n_devices;
    int i;
    Display * display = QX11Info::display();
    device_info = XListInputDevices (display, &n_devices);
    for (i = 0; i < n_devices; i++) {
        set_middle_button (&device_info[i], middle_button);
    }

    if (device_info != NULL)
        XFreeDeviceList (device_info);
}


void set_locate_pointer (MouseManager *manager, bool     state)
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
            manager->settings_mouse->set(KEY_MOUSE_LOCATE_POINTER,false);
            g_error_free (error);
        }

    }
    else if (manager->locate_pointer_spawned) {
            kill (manager->locate_pointer_pid, SIGHUP);
            g_spawn_close_pid (manager->locate_pointer_pid);
            manager->locate_pointer_spawned = FALSE;
    }
}

void MouseManager::mouse_callback (QString keys)
{
    if (keys.compare(QString::fromLocal8Bit(KEY_LEFT_HANDED))==0){
        bool mouse_left_handed = settings_mouse->get(keys).toBool();
        bool touchpad_left_handed = get_touchpad_handedness (mouse_left_handed);
        set_left_handed_all (this, mouse_left_handed, touchpad_left_handed);

    } else if ((keys.compare(QString::fromLocal8Bit(KEY_MOTION_ACCELERATION))==0)
               || (keys.compare(QString::fromLocal8Bit(KEY_MOTION_THRESHOLD))==0)){
        set_motion_all (this);

    } else if (keys.compare(QString::fromLocal8Bit(KEY_MIDDLE_BUTTON_EMULATION))==0){
        set_middle_button_all (settings_mouse->get(keys).toBool());

    } else if (keys.compare(QString::fromLocal8Bit(KEY_MOUSE_LOCATE_POINTER))==0){
        set_locate_pointer (this, settings_mouse->get(keys).toBool());
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
void set_disable_w_typing_synaptics (MouseManager *manager,
                                     bool         state)
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
                manager->settings_touchpad->set(KEY_TOUCHPAD_DISABLE_W_TYPING,false);
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
        CT_SYSLOG(LOG_ERR,"%s:error",__func__);
    }
}

void set_disable_w_typing_libinput (MouseManager *manager,
                                    bool         state)
{
    XDeviceInfo *device_info;
    int n_devices;
    int i;

    /* This is only called once for synaptics but for libinput
     * we need to loop through the list of devices
     */
    device_info = XListInputDevices (QX11Info::display(), &n_devices);

    for (i = 0; i < n_devices; i++) {
        touchpad_set_bool (&device_info[i], "libinput Disable While Typing Enabled", 0, state);
    }

    if (device_info != NULL)
        XFreeDeviceList (device_info);
}

void set_disable_w_typing (MouseManager *manager,
                           bool         state)
{
    if (property_from_name ("Synaptics Off"))
        set_disable_w_typing_synaptics (manager, state);

    if (property_from_name ("libinput Disable While Typing Enabled"))
        set_disable_w_typing_libinput (manager, state);
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


void set_tap_to_click_all (MouseManager *manager)
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);

    if (devicelist == NULL)
            return;

    bool state = manager->settings_touchpad->get(KEY_TOUCHPAD_TAP_TO_CLICK).toBool();
    bool left_handed = manager->get_touchpad_handedness (manager->settings_mouse->get(KEY_LEFT_HANDED).toBool());
    int one_finger_tap = manager->settings_touchpad->get(KEY_TOUCHPAD_ONE_FINGER_TAP).toInt();
    int two_finger_tap = manager->settings_touchpad->get(KEY_TOUCHPAD_TWO_FINGER_TAP).toBool();
    int three_finger_tap = manager->settings_touchpad->get(KEY_TOUCHPAD_THREE_FINGER_TAP).toBool();

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
    Display *display = QX11Info::display();
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
        CT_SYSLOG(LOG_ERR,"Error in setting scroll method on \"%s\"", device_info->name);
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

void set_scrolling_all (QGSettings *settings)
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
    Display *display = QX11Info::display();
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
        CT_SYSLOG(LOG_ERR,"Error in setting natural scroll on \"%s\"", device_info->name);
    }
}

void set_natural_scroll_libinput (XDeviceInfo *device_info,
                                  bool       natural_scroll)
{
    qDebug ("Trying to set %s for \"%s\"",
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

void set_natural_scroll_all (MouseManager *manager)
{
    int numdevices, i;
    XDeviceInfo *devicelist = XListInputDevices (QX11Info::display(), &numdevices);

    if (devicelist == NULL)
            return;
    bool natural_scroll = manager->settings_touchpad->get(KEY_TOUCHPAD_NATURAL_SCROLL).toBool();
    for (i = 0; i < numdevices; i++) {
            set_natural_scroll (&devicelist[i], natural_scroll);
    }
    XFreeDeviceList (devicelist);
}

void set_touchpad_enabled (XDeviceInfo *device_info,
                           bool         state)
{
    XDevice *device;
    Atom prop_enabled;
    unsigned char data = state;
    Display *display =  gdk_x11_get_default_xdisplay ();//QX11Info::display();//

    prop_enabled = property_from_name ("Device Enabled");
    if (!prop_enabled)
        return;

    device = device_is_touchpad (device_info);
    if (device == NULL) {
        return;
    }
    try {
        XChangeDeviceProperty (display, device,
                               prop_enabled, XA_INTEGER, 8,
                               PropModeReplace, &data, 1);

        XCloseDevice (display, device);
        gdk_display_flush (gdk_display_get_default());
    } catch (int x) {
        CT_SYSLOG(LOG_ERR,"Error %s device \"%s\"",
                  (state) ? "enabling" : "disabling",
                   device_info->name);
    }

}

void set_touchpad_enabled_all (bool state)
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

void MouseManager::touchpad_callback (QString keys)
{

    if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_DISABLE_W_TYPING))==0) {
            set_disable_w_typing (this, settings_touchpad->get(keys).toBool());
    } else if (keys.compare(QString::fromLocal8Bit(KEY_LEFT_HANDED))== 0) {
            bool mouse_left_handed = settings_mouse->get(keys).toBool();
            bool touchpad_left_handed = get_touchpad_handedness (mouse_left_handed);
            set_left_handed_all (this, mouse_left_handed, touchpad_left_handed);
    } else if ((keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_TAP_TO_CLICK))    == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_ONE_FINGER_TAP))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_TWO_FINGER_TAP))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_THREE_FINGER_TAP))== 0)) {
            set_tap_to_click_all (this);
/* Do not set click actions since ukwm take over this
*        } else if ((keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_TWO_FINGER_CLICK)) == 0)
*                 ||(keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_THREE_FINGER_CLICK)) == 0)) {
*               set_click_actions_all (this);
*/
    } else if ((keys.compare(QString::fromLocal8Bit(KEY_VERT_EDGE_SCROLL))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_HORIZ_EDGE_SCROLL)) == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_VERT_TWO_FINGER_SCROLL))  == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_HORIZ_TWO_FINGER_SCROLL)) == 0)) {
            set_scrolling_all (this->settings_touchpad);
    } else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_NATURAL_SCROLL)) == 0) {
            set_natural_scroll_all (this);
    } else if (keys.compare(QString::fromLocal8Bit(KEY_TOUCHPAD_ENABLED)) == 0) {
            set_touchpad_enabled_all (settings_touchpad->get(keys).toBool());
    } else if ((keys.compare(QString::fromLocal8Bit(KEY_MOTION_ACCELERATION)) == 0)
            || (keys.compare(QString::fromLocal8Bit(KEY_MOTION_THRESHOLD)) == 0)) {
            set_motion_all (this);
    }
}

void set_mouse_settings (MouseManager *manager)
{
    bool mouse_left_handed = manager->settings_mouse->get(KEY_LEFT_HANDED).toBool();
    bool touchpad_left_handed = manager->get_touchpad_handedness (mouse_left_handed);

    set_left_handed_all (manager, mouse_left_handed, touchpad_left_handed);

    set_motion_all (manager);
    set_middle_button_all (manager->settings_mouse->get(KEY_MIDDLE_BUTTON_EMULATION).toBool());

    set_disable_w_typing (manager, manager->settings_touchpad->get(KEY_TOUCHPAD_DISABLE_W_TYPING).toBool());

    set_tap_to_click_all (manager);
/* Do not set click actions since ukwm take over this
*
*       set_click_actions_all (manager);
*/
    set_scrolling_all (manager->settings_touchpad);
    set_natural_scroll_all (manager);
    set_touchpad_enabled_all (manager->settings_touchpad->get(KEY_TOUCHPAD_ENABLED).toBool());
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
    display = QX11Info::display();//gdk_x11_get_default_xdisplay ();

    gdk_x11_display_error_trap_push (gdk_display_get_default());
    DevicePresence (display, xi_presence, class_presence);
    XSelectExtensionEvent (display,
                           RootWindow (display, DefaultScreen (display)),
                           &class_presence, 1);

    gdk_display_flush (gdk_display_get_default());
    if (!gdk_x11_display_error_trap_pop (gdk_display_get_default()))
        gdk_window_add_filter (NULL, devicepresence_filter, manager);
}

void MouseManager::usd_mouse_manager_idle_cb()
{

    time->stop();

    QObject::connect(settings_mouse,SIGNAL(changed(QString)),
                     this,SLOT(mouse_callback(QString)));
    QObject::connect(settings_touchpad,SIGNAL(changed(QString)),
                     this,SLOT(touchpad_callback(QString)));
    syndaemon_spawned = FALSE;
    set_devicepresence_handler (this);
    set_mouse_settings (this);
    set_locate_pointer (this, settings_mouse->get(KEY_MOUSE_LOCATE_POINTER).toBool());
}


