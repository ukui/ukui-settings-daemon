#include "xrandr-manager.h"
#include "clib-syslog.h"
#include <QDBusError>
#include <QDBusConnectionInterface>
#include <QString>
#include <QDBusConnection>

#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <assert.h>
#include <libintl.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#define USD_XRANDR_ICON_NAME "uksd-xrandr"

#define CONF_SCHEMA     "org.ukui.SettingsDaemon.plugins.xrandr"
#define CONF_KEY_SHOW_NOTIFICATION_ICON     "show-notification-icon"
#define CONF_KEY_USE_XORG_MONITOR_SETTINGS             "use-xorg-monitor-settings"
#define CONF_KEY_TURN_ON_EXTERNAL_MONITORS_AT_STARTUP  "turn-on-external-monitors-at-startup"
#define CONF_KEY_TURN_ON_LAPTOP_MONITOR_AT_STARTUP     "turn-on-laptop-monitor-at-startup"
#define CONF_KEY_DEFAULT_CONFIGURATION_FILE            "default-configuration-file"

static const MateRRRotation possible_rotations[] = { 
        MATE_RR_ROTATION_0,
        MATE_RR_ROTATION_90,
        MATE_RR_ROTATION_180,
        MATE_RR_ROTATION_270
        /* We don't allow REFLECT_X or REFLECT_Y for now, as mate-display-properties doesn't allow them, either */
};

struct confirmation {
        XrandrManager *managers;
        GdkWindow *parent_window;
        guint32 timestamp;
};

struct _TimeoutDialog{
        XrandrManager *managers;
        GtkWidget *dialog;

        int countdown;
        int response_id;
};

XrandrManager * XrandrManager::mXrandrManager = nullptr;

static XrandrManager * manager = XrandrManager::XrandrManagerNew();
static bool RegisterManagerDbus(XrandrManager& m);

static FILE *log_file;

XrandrManager::XrandrManager()
{

}

XrandrManager::~XrandrManager()
{
    if(mXrandrManager)
        delete mXrandrManager;
}

XrandrManager * XrandrManager::XrandrManagerNew()
{
    if (nullptr == mXrandrManager)
        mXrandrManager = new XrandrManager();

    if (RegisterManagerDbus(*mXrandrManager))
       return nullptr;

    return mXrandrManager;
}

bool RegisterManagerDbus(XrandrManager& m)
{
    QString XrandrDbusName = USD_XRANDR_DBUS_NAME;
    if (QDBusConnection::sessionBus().interface()->isServiceRegistered(XrandrDbusName)){
        return false;
    }
    QDBusConnection bus = QDBusConnection::sessionBus();

    if (!bus.registerService(USD_XRANDR_DBUS_NAME)) {
        CT_SYSLOG(LOG_ERR, "error Xrandr Name bus: '%s'", bus.lastError().message().toUtf8().data());
        return false;
    }
    if (!bus.registerObject(USD_XRANDR_DBUS_PATH, (QObject*)&m, QDBusConnection::ExportAllSlots)) {
        CT_SYSLOG(LOG_ERR, "path xrandr error: '%s'", bus.lastError().message().toUtf8().data());
        return false;
    }
    CT_SYSLOG(LOG_DEBUG, "Xrandr RegisterManagerDbus successful!");

    return true;
}

bool XrandrManager::XrandrManagerStart(GError** error)
{
    CT_SYSLOG(LOG_DEBUG,"Start Xrandr Manager");
    log_open();
    log_msg ("------------------------------------\nSTARTING XRANDR PLUGIN\n");
    manager->rw_screen = mate_rr_screen_new (gdk_screen_get_default (), error);
    if (manager->rw_screen == NULL ){
        log_msg("Could not initialize the RANDR plugin%s%s\n",
                (error && *error) ? ": " : "",
                (error && *error) ? (*error)->message : "");
        log_close();
        return FALSE;
    }

    g_signal_connect (manager->rw_screen, "changed", G_CALLBACK (on_randr_event), manager);

    log_msg("State of screen at startup:\n");
    log_screen(manager->rw_screen);

    manager->running = TRUE;
    manager->settings = g_settings_new(CONF_SCHEMA);

    g_signal_connect (manager->settings,
                      "changed::" CONF_KEY_SHOW_NOTIFICATION_ICON,
                      G_CALLBACK (on_config_changed),
                      manager);

    if (switch_video_mode_keycode) {
            gdk_error_trap_push ();

            XGrabKey (gdk_x11_get_default_xdisplay(),
                      switch_video_mode_keycode, AnyModifier,
                      gdk_x11_get_default_root_xwindow(),
                      True, GrabModeAsync, GrabModeAsync);

            gdk_flush ();
            gdk_error_trap_pop_ignored ();
    }
    if (rotate_windows_keycode) {
        gdk_error_trap_push ();

        XGrabKey (gdk_x11_get_default_xdisplay(),
                  rotate_windows_keycode, AnyModifier,
                  gdk_x11_get_default_root_xwindow(),
                  True, GrabModeAsync, GrabModeAsync);

        gdk_flush ();
        gdk_error_trap_pop_ignored ();
    }
    show_timestamps_dialog ("Startup");

    if (!apply_stored_configuration_at_startup (manager,GDK_CURRENT_TIME)) // we don't have a real timestamp at startup anyway
            if (!apply_default_configuration_from_file (manager,GDK_CURRENT_TIME))
                    if (!g_settings_get_boolean (manager->settings, CONF_KEY_USE_XORG_MONITOR_SETTINGS))
                            apply_default_boot_configuration (manager,GDK_CURRENT_TIME);

    log_msg ("State of screen after initial configuration:\n");
    log_screen (manager->rw_screen);

    gdk_window_add_filter (gdk_get_default_root_window(),
                           (GdkFilterFunc)event_filter,
                           manager);

    start_or_stop_icon (manager);

    log_close ();

    return TRUE;
}

void XrandrManager::XrandrManagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"Stoping Xrandr manager");
    manager->running=FALSE;
    if (manager->switch_video_mode_keycode) {
        gdk_error_trap_push ();

        XUngrabKey (gdk_x11_get_default_xdisplay(),
                    manager->switch_video_mode_keycode, AnyModifier,
                    gdk_x11_get_default_root_xwindow());

        gdk_error_trap_pop_ignored ();
    }
    if (manager->rotate_windows_keycode) {
        gdk_error_trap_push ();

        XUngrabKey (gdk_x11_get_default_xdisplay(),
                manager->rotate_windows_keycode, AnyModifier,
                gdk_x11_get_default_root_xwindow());

        gdk_error_trap_pop_ignored ();
    }

    gdk_window_remove_filter (gdk_get_default_root_window (),
                              (GdkFilterFunc) event_filter,
                              manager);
    if (manager->settings != NULL) {
            g_object_unref (manager->settings);
            manager->settings = NULL;
    }

    if (manager->rw_screen != NULL) {
            g_object_unref (manager->rw_screen);
            manager->rw_screen = NULL;
    }

    status_icon_stop (manager);

    log_open ();
    log_msg ("STOPPING XRANDR PLUGIN\n------------------------------\n");
    log_close ();
}


void XrandrManager::log_open()
{
    char *toggle_filename;
    char *log_filename;
    struct stat st;
    if (log_file)
        return;
    toggle_filename = g_build_filename (g_get_home_dir (), "usd-debug-randr", NULL);
    log_filename = g_build_filename (g_get_home_dir (), "usd-debug-randr.log", NULL);

    if  (stat (toggle_filename, &st) != 0)
        goto out;

    log_file = fopen (log_filename, "a");

    if (log_file && ftell (log_file) == 0)
        fprintf (log_file, "To keep this log from being created, please rm ~/usd-debug-randr\n");
    out:
        g_free (toggle_filename);
        g_free (log_filename);
}

void XrandrManager::log_close() 
{
    if (log_file) {
        fclose (log_file);
        log_file = NULL;
    }
}

void XrandrManager::log_msg(const char *format, ...)
{
    if (log_file) {
        va_list args;

        va_start (args, format);
        vfprintf (log_file, format, args);
        va_end (args);
    }
}

void XrandrManager::log_output(MateRROutputInfo *output)
{
    gchar *name = mate_rr_output_info_get_name (output);
    gchar *display_name = mate_rr_output_info_get_display_name (output);

    log_msg ("        %s: ", name ? name : "unknown");

    if (mate_rr_output_info_is_connected (output)) {
        if (mate_rr_output_info_is_active (output)) {
            int x, y, width, height;
            mate_rr_output_info_get_geometry (output, &x, &y, &width, &height);
            log_msg ("%dx%d@%d +%d+%d",
                     width,
                     height,
                     mate_rr_output_info_get_refresh_rate (output),
                     x,
                     y);
        } else
            log_msg ("off");
    } else
        log_msg ("disconnected");

    if (display_name)
        log_msg (" (%s)", display_name);

    if (mate_rr_output_info_get_primary (output))
        log_msg (" (primary output)");
    log_msg ("\n");
}

void XrandrManager::log_configuration(MateRRConfig *config)
{
    int i;
    MateRROutputInfo **outputs = mate_rr_config_get_outputs (config);

    log_msg ("        cloned: %s\n", mate_rr_config_get_clone (config) ? "yes" : "no");

    for (i = 0; outputs[i] != NULL; i++)
        log_output (outputs[i]);

    if (i == 0)
        log_msg ("        no outputs!\n");
}

char XrandrManager::timestamp_relationship(unsigned int a, unsigned int b)
{
    if (a < b)
        return '<';
    else if (a > b)
        return '>';
    else
        return '=';
}

void XrandrManager::log_screen(MateRRScreen *screen)
{
    MateRRConfig *config;
    int min_w, min_h, max_w, max_h;
    unsigned int change_timestamp, config_timestamp;

    if (!log_file)
            return;

    config = mate_rr_config_new_current (screen, NULL);

    mate_rr_screen_get_ranges (screen, &min_w, &max_w, &min_h, &max_h);
    mate_rr_screen_get_timestamps (screen, &change_timestamp, &config_timestamp);

    log_msg ("        Screen min(%d, %d), max(%d, %d), change=%u %c config=%u\n",
             min_w, min_h,
             max_w, max_h,
             change_timestamp,
             timestamp_relationship (change_timestamp, config_timestamp),
             config_timestamp);

    log_configuration (config);
    g_object_unref (config);
}



GdkFilterReturn XrandrManager::event_filter(GdkXEvent *xevent, GdkEvent *event, gpointer data)
{
    XrandrManager *manager = ( XrandrManager *)data;
    XEvent *xev = (XEvent *) xevent;

    if (!manager->running)
            return GDK_FILTER_CONTINUE;

    /* verify we have a key event */
    if (xev->xany.type != KeyPress && xev->xany.type != KeyRelease)
            return GDK_FILTER_CONTINUE;

    if (xev->xany.type == KeyPress) {
            if (xev->xkey.keycode == manager->switch_video_mode_keycode)
                    handle_fn_f7 (manager, xev->xkey.time);
            else if (xev->xkey.keycode == manager->rotate_windows_keycode)
                    handle_rotate_windows (manager, xev->xkey.time);

            return GDK_FILTER_CONTINUE;
    }

    return GDK_FILTER_CONTINUE;
}

void XrandrManager::handle_rotate_windows (XrandrManager *mgr, guint32 timestamp)
{
        MateRRScreen *screen = mgr->rw_screen;
        MateRRConfig *current;
        MateRROutputInfo *rotatable_output_info;
        int num_allowed_rotations;
        MateRRRotation allowed_rotations;
        MateRRRotation next_rotation;

        CT_SYSLOG(LOG_DEBUG,"Handling XF86RotateWindows");

        /* Which output? */

        current = mate_rr_config_new_current (screen, NULL);

        rotatable_output_info = get_laptop_output_info (screen, current);
        if (rotatable_output_info == NULL) {
                CT_SYSLOG(LOG_DEBUG,"No laptop outputs found to rotate; XF86RotateWindows key will do nothing");
                goto out;
        }

        /* Which rotation? */

        get_allowed_rotations_for_output (current, mgr->rw_screen, rotatable_output_info, &num_allowed_rotations, &allowed_rotations);
        next_rotation = get_next_rotation (allowed_rotations, mate_rr_output_info_get_rotation (rotatable_output_info));

        if (next_rotation == mate_rr_output_info_get_rotation (rotatable_output_info)) {
                CT_SYSLOG(LOG_DEBUG,"No rotations are supported other than the current one; XF86RotateWindows key will do nothing");
                goto out;
        }

        /* Rotate */

        mate_rr_output_info_set_rotation (rotatable_output_info, next_rotation);

        apply_configuration_and_display_error (mgr, current, timestamp);

out:
        g_object_unref (current);
}

MateRRRotation XrandrManager::get_next_rotation (MateRRRotation allowed_rotations, MateRRRotation current_rotation)
{
    int i;
    int current_index;

    /* First, find the index of the current rotation */

    current_index = -1;

    for (i = 0; i < G_N_ELEMENTS (possible_rotations); i++) {
        MateRRRotation r;

        r = possible_rotations[i];
        if (r == current_rotation) {
                current_index = i;
                break;
        }
    }

    if (current_index == -1) {
        /* Huh, the current_rotation was not one of the supported rotations.  Bail out. */
        return current_rotation;
    }

    /* Then, find the next rotation that is allowed */
    i = (current_index + 1) % G_N_ELEMENTS (possible_rotations);

    while (1) {
        MateRRRotation r;

        r = possible_rotations[i];
        if (r == current_rotation) {
                /* We wrapped around and no other rotation is suported.  Bummer. */
                return current_rotation;
        } else if (r & allowed_rotations)
                return r;

        i = (i + 1) % G_N_ELEMENTS (possible_rotations);
    }
}

MateRROutputInfo * XrandrManager::get_laptop_output_info (MateRRScreen *screen, MateRRConfig *config)
{
    int i;
    MateRROutputInfo **outputs = mate_rr_config_get_outputs (config);

    for (i = 0; outputs[i] != NULL; i++) {
        if (is_laptop (screen, outputs[i]))
            return outputs[i];
    }

    return NULL;
}


void XrandrManager::handle_fn_f7(XrandrManager *mgr, guint32 timestamp)
{
    MateRRScreen *screen = mgr->rw_screen;
    MateRRConfig *current;
    GError *error;

    /* Theory of fn-F7 operation
     *
     * We maintain a datastructure "fn_f7_status", that contains
     * a list of MateRRConfig's. Each of the MateRRConfigs has a
     * mode (or "off") for each connected output.
     *
     * When the user hits fn-F7, we cycle to the next MateRRConfig
     * in the data structure. If the data structure does not exist, it
     * is generated. If the configs in the data structure do not match
     * the current hardware reality, it is regenerated.
     *
     */
    CT_SYSLOG(LOG_DEBUG,"Handling fn-f7");

    log_open ();
    log_msg ("Handling XF86Display hotkey - timestamp %u\n", timestamp);

    error = NULL;
    if (!mate_rr_screen_refresh (screen, &error) && error) {
            char *str;
                                    /*String Chinese localisation is not handled*/
            str = g_strdup_printf ("Could not refresh the screen information: %s", error->message);
            g_error_free (error);

            log_msg ("%s\n", str);          /*String Chinese localisation is not handled*/
            error_message (mgr, str, NULL, "Trying to switch the monitor configuration anyway.");
            g_free (str);
    }

    if (!mgr->fn_f7_configs) {
            log_msg ("Generating stock configurations:\n");
            generate_fn_f7_configs (mgr);
            log_configurations (mgr->fn_f7_configs);
    }

    current = mate_rr_config_new_current (screen, NULL);
    gboolean equal = mate_rr_config_equal (current, mgr->fn_f7_configs[mgr->current_fn_f7_config]);
    gboolean match = mate_rr_config_match (current, mgr->fn_f7_configs[0]);
    if (mgr->fn_f7_configs && (!match || ! equal)) {
                /* Our view of the world is incorrect, so regenerate the
                 * configurations
                 */
                generate_fn_f7_configs (mgr);
                log_msg ("Regenerated stock configurations:\n");
                log_configurations (mgr->fn_f7_configs);
        }

    g_object_unref (current);

    if (mgr->fn_f7_configs) {
            guint32 server_timestamp;
            gboolean success;

            mgr->current_fn_f7_config++;

            if (mgr->fn_f7_configs[mgr->current_fn_f7_config] == NULL)
                    mgr->current_fn_f7_config = 0;

            CT_SYSLOG(LOG_DEBUG,"cycling to next configuration (%d)", mgr->current_fn_f7_config);

            print_configuration (mgr->fn_f7_configs[mgr->current_fn_f7_config], "new config");

            CT_SYSLOG(LOG_DEBUG,"applying");

            /* See https://bugzilla.gnome.org/show_bug.cgi?id=610482
             *
             * Sometimes we'll get two rapid XF86Display keypress events,
             * but their timestamps will be out of order with respect to the
             * RANDR timestamps.  This *may* be due to stupid BIOSes sending
             * out display-switch keystrokes "to make Windows work".
             *
             * The X server will error out if the timestamp provided is
             * older than a previous change configuration timestamp. We
             * assume here that we do want this event to go through still,
             * since kernel timestamps may be skewed wrt the X server.
             */
            mate_rr_screen_get_timestamps (screen, NULL, &server_timestamp);
            if (timestamp < server_timestamp)
                    timestamp = server_timestamp;

            success = apply_configuration_and_display_error (mgr, mgr->fn_f7_configs[mgr->current_fn_f7_config], timestamp);

            if (success) {
                    log_msg ("Successfully switched to configuration (timestamp %u):\n", timestamp);
                    log_configuration (mgr->fn_f7_configs[mgr->current_fn_f7_config]);
            }
    }
    else {
            CT_SYSLOG(LOG_DEBUG,"no configurations generated");
    }

    log_close ();

    CT_SYSLOG(LOG_DEBUG,"done handling fn-f7");
}

void XrandrManager::log_configurations (MateRRConfig **configs)
{
        int i;

        if (!configs) {
                log_msg ("    No configurations\n");
                return;
        }

        for (i = 0; configs[i]; i++) {
                log_msg ("    Configuration %d\n", i);
                log_configuration (configs[i]);
        }
}

void XrandrManager::generate_fn_f7_configs(XrandrManager *mgr)
{
        GPtrArray *array = g_ptr_array_new ();
        MateRRScreen *screen = mgr->rw_screen;

        CT_SYSLOG(LOG_DEBUG,"Generating configurations");

        /* Free any existing list of configurations */
        if (mgr->fn_f7_configs) {
                int i;

                for (i = 0; mgr->fn_f7_configs[i] != NULL; ++i)
                        g_object_unref (mgr->fn_f7_configs[i]);
                g_free (mgr->fn_f7_configs);

                mgr->fn_f7_configs = NULL;
                mgr->current_fn_f7_config = -1;
        }

        g_ptr_array_add (array, mate_rr_config_new_current (screen, NULL));
        g_ptr_array_add (array, make_clone_setup (screen));
        g_ptr_array_add (array, make_xinerama_setup (screen));
        g_ptr_array_add (array, make_laptop_setup (screen));
        g_ptr_array_add (array, make_other_setup (screen));

        array = sanitize (mgr, array);

        if (array) {
                mgr->fn_f7_configs = (MateRRConfig **)g_ptr_array_free (array, FALSE);
                mgr->current_fn_f7_config = 0;
        }
}

GPtrArray * XrandrManager::sanitize (XrandrManager *manager, GPtrArray *array)
{
    int i;
    GPtrArray * new1;

    CT_SYSLOG(LOG_DEBUG,"before sanitizing");

    for (i = 0; i < array->len; ++i) {
        if (array->pdata[i]) {
            print_configuration ( (MateRRConfig *)array->pdata[i], "before");
        }
    }


    /* Remove configurations that are duplicates of
     * configurations earlier in the cycle
     */
    for (i = 0; i < array->len; i++) {
        int j;

        for (j = i + 1; j < array->len; j++) {
            MateRRConfig *this1 =  (MateRRConfig *)array->pdata[j];
            MateRRConfig *other =  (MateRRConfig *)array->pdata[i];

            if (this1 && other && mate_rr_config_equal (this1, other)) {
                CT_SYSLOG(LOG_DEBUG,"removing duplicate configuration");
                g_object_unref (this1);
                array->pdata[j] = NULL;
                break;
            }
        }
    }

    for (i = 0; i < array->len; ++i) {
        MateRRConfig *config =  (MateRRConfig *)array->pdata[i];

        if (config && config_is_all_off (config)) {
            CT_SYSLOG(LOG_DEBUG,"removing configuration as all outputs are off");
            g_object_unref (array->pdata[i]);
            array->pdata[i] = NULL;
        }
    }

    /* Do a final sanitization pass.  This will remove configurations that
     * don't fit in the framebuffer's Virtual size.
     */

    for (i = 0; i < array->len; i++) {
        MateRRConfig *config = (MateRRConfig *)array->pdata[i];

        if (config) {
            GError *error;

            error = NULL;
            if (!mate_rr_config_applicable (config, manager->rw_screen, &error)) { /* NULL-GError */
                CT_SYSLOG(LOG_DEBUG,"removing configuration which is not applicable because %s", error->message);
                g_error_free (error);

                g_object_unref (config);
                array->pdata[i] = NULL;
            }
        }
    }

    /* Remove NULL configurations */
    new1 = g_ptr_array_new ();

    for (i = 0; i < array->len; ++i) {
        if (array->pdata[i]) {
                g_ptr_array_add (new1, array->pdata[i]);
                print_configuration ( (MateRRConfig *)array->pdata[i], "Final");
        }
    }

    if (new1->len > 0) {
        g_ptr_array_add (new1, NULL);
    } else {
        g_ptr_array_free (new1, TRUE);
        new1 = NULL;
    }

    g_ptr_array_free (array, TRUE);

    return new1;
}


void XrandrManager::apply_default_boot_configuration(XrandrManager *mgr, guint32 timestamp)
{
    MateRRScreen *screen = mgr->rw_screen;
    MateRRConfig *config;
    gboolean turn_on_external, turn_on_laptop;

    turn_on_external =
            g_settings_get_boolean (mgr->settings, CONF_KEY_TURN_ON_EXTERNAL_MONITORS_AT_STARTUP);
    turn_on_laptop =
            g_settings_get_boolean (mgr->settings, CONF_KEY_TURN_ON_LAPTOP_MONITOR_AT_STARTUP);

    if (turn_on_external && turn_on_laptop)
            config = make_clone_setup (screen);
    else if (!turn_on_external && turn_on_laptop)
            config = make_laptop_setup (screen);
    else if (turn_on_external && !turn_on_laptop)
            config = make_other_setup (screen);
    else
            config = make_laptop_setup (screen);

    if (config) {
            apply_configuration_and_display_error (mgr,config, timestamp);
            g_object_unref (config);
    }
}

MateRRConfig * XrandrManager::make_xinerama_setup(MateRRScreen *screen)
{
    /* Turn on everything that has a preferred mode, and
    * position it from left to right
    */
    MateRRConfig *result = mate_rr_config_new_current (screen, NULL);
    MateRROutputInfo **outputs = mate_rr_config_get_outputs (result);
    int i;
    int x;

    x = 0;
    for (i = 0; outputs[i] != NULL; ++i) {
        MateRROutputInfo *info = outputs[i];

        if (is_laptop (screen, info))
                x = turn_on_and_get_rightmost_offset (screen, info, x);
    }

    for (i = 0; outputs[i] != NULL; ++i) {
        MateRROutputInfo *info = outputs[i];

        if (mate_rr_output_info_is_connected (info) && !is_laptop (screen, info))
                x = turn_on_and_get_rightmost_offset (screen, info, x);
    }

    if (config_is_all_off (result)) {
        g_object_unref (G_OBJECT (result));
        result = NULL;
    }

    print_configuration (result, "xinerama setup");

    return result;
}

int XrandrManager::turn_on_and_get_rightmost_offset (MateRRScreen *screen, MateRROutputInfo *info, int x)
{
        if (turn_on (screen, info, x, 0)) {
                int width;
                mate_rr_output_info_get_geometry (info, NULL, NULL, &width, NULL);
                x += width;
        }

        return x;
}

MateRRConfig * XrandrManager::make_other_setup (MateRRScreen *screen)
{
    /* Turn off all laptops, and make all external monitors clone
     * from (0, 0)
     */

    MateRRConfig *result = mate_rr_config_new_current (screen, NULL);
    MateRROutputInfo **outputs = mate_rr_config_get_outputs (result);
    int i;

    for (i = 0; outputs[i] != NULL; ++i) {
        MateRROutputInfo *info = outputs[i];

        if (is_laptop (screen, info)) {
                mate_rr_output_info_set_active (info, FALSE);
        }
        else {
                if (mate_rr_output_info_is_connected (info))
                        turn_on (screen, info, 0, 0);
       }
    }

    if (config_is_all_off (result)) {
        g_object_unref (G_OBJECT (result));
        result = NULL;
    }

    print_configuration (result, "other setup");

    return result;
}



MateRRConfig * XrandrManager::make_laptop_setup(MateRRScreen *screen)
{
    /* Turn on the laptop, disable everything else */
    MateRRConfig *result = mate_rr_config_new_current (screen, NULL);
    MateRROutputInfo **outputs = mate_rr_config_get_outputs (result);
    int i;

    for (i = 0; outputs[i] != NULL; ++i) {
        MateRROutputInfo *info = outputs[i];
        if (is_laptop (screen, info)) {
                if (!turn_on (screen, info, 0, 0)) {
                        g_object_unref (G_OBJECT (result));
                        result = NULL;
                        break;
                }
        }
        else {
                mate_rr_output_info_set_active (info, FALSE);
        }
    }

    if (result && config_is_all_off (result)) {
            g_object_unref (G_OBJECT (result));
            result = NULL;
    }

    print_configuration (result, "Laptop setup");

    /* FIXME - Maybe we should return NULL if there is more than
     * one connected "laptop" screen?
     */
    return result;
}

gboolean XrandrManager::is_laptop (MateRRScreen *screen, MateRROutputInfo *output)
{
        MateRROutput *rr_output;

        rr_output = mate_rr_screen_get_output_by_name (screen, mate_rr_output_info_get_name (output));
        return mate_rr_output_is_laptop (rr_output);
}
gboolean XrandrManager::turn_on(MateRRScreen *screen, MateRROutputInfo *info, int x, int y)
{
    MateRROutput *output = mate_rr_screen_get_output_by_name (screen, mate_rr_output_info_get_name (info));
    MateRRMode *mode = find_best_mode (output);

    if (mode) {
            mate_rr_output_info_set_active (info, TRUE);
            mate_rr_output_info_set_geometry (info, x, y, mate_rr_mode_get_width (mode), mate_rr_mode_get_height (mode));
            mate_rr_output_info_set_rotation (info, MATE_RR_ROTATION_0);
            mate_rr_output_info_set_refresh_rate (info, mate_rr_mode_get_freq (mode));

            return TRUE;
    }

    return FALSE;
}

MateRRMode * XrandrManager::find_best_mode (MateRROutput *output)
{
    MateRRMode *preferred;
    MateRRMode **modes;
    int best_size;
    int best_width, best_height, best_rate;
    int i;
    MateRRMode *best_mode;

    preferred = mate_rr_output_get_preferred_mode (output);
    if (preferred)
            return preferred;

    modes = mate_rr_output_list_modes (output);
    if (!modes)
            return NULL;

    best_size = best_width = best_height = best_rate = 0;
    best_mode = NULL;

    for (i = 0; modes[i] != NULL; i++) {
        int w, h, r;
        int size;

        w = mate_rr_mode_get_width (modes[i]);
        h = mate_rr_mode_get_height (modes[i]);
        r = mate_rr_mode_get_freq  (modes[i]);

        size = w * h;

        if (size > best_size) {
                best_size   = size;
                best_width  = w;
                best_height = h;
                best_rate   = r;
                best_mode   = modes[i];
        } else if (size == best_size) {
                if (r > best_rate) {
                        best_rate = r;
                        best_mode = modes[i];
                }
        }
    }

    return best_mode;
}



MateRRConfig * XrandrManager::make_clone_setup(MateRRScreen *screen)
{
    MateRRConfig *result;
    MateRROutputInfo **outputs;
    int width, height;
    int i;

    if (!get_clone_size (screen, &width, &height))
            return NULL;

    result = mate_rr_config_new_current (screen, NULL);
    outputs = mate_rr_config_get_outputs (result);

    for (i = 0; outputs[i] != NULL; ++i) {
            MateRROutputInfo *info = outputs[i];

        mate_rr_output_info_set_active (info, FALSE);
        if (mate_rr_output_info_is_connected (info)) {
            MateRROutput *output =
                mate_rr_screen_get_output_by_name (screen, mate_rr_output_info_get_name (info));
            MateRRMode **modes = mate_rr_output_list_modes (output);
            int j;
            int best_rate = 0;

            for (j = 0; modes[j] != NULL; ++j) {
                MateRRMode *mode = modes[j];
                int w, h;

                w = mate_rr_mode_get_width (mode);
                h = mate_rr_mode_get_height (mode);

                if (w == width && h == height) {
                    int r = mate_rr_mode_get_freq (mode);
                    if (r > best_rate)
                          best_rate = r;
                }
            }

            if (best_rate > 0) {
                    mate_rr_output_info_set_active (info, TRUE);
                    mate_rr_output_info_set_rotation (info, MATE_RR_ROTATION_0);
                    mate_rr_output_info_set_refresh_rate (info, best_rate);
                    mate_rr_output_info_set_geometry (info, 0, 0, width, height);
            }
        }
    }

    if (config_is_all_off (result)) {
            g_object_unref (result);
            result = NULL;
    }

    print_configuration (result, "clone setup");

    return result;
}

gboolean XrandrManager::get_clone_size(MateRRScreen *screen, int *width, int *height)
{
    MateRRMode **modes = mate_rr_screen_list_clone_modes (screen);
    int best_w, best_h;
    int i;

    best_w = 0;
    best_h = 0;
    for (i = 0; modes[i] != NULL; ++i) {
            MateRRMode *mode = modes[i];
            int w, h;
            w = mate_rr_mode_get_width (mode);
            h = mate_rr_mode_get_height (mode);
            if (w * h > best_w * best_h) {
                    best_w = w;
                    best_h = h;
            }
    }
    if (best_w > 0 && best_h > 0) {
            if (width)
                    *width = best_w;
            if (height)
                    *height = best_h;
            return TRUE;
    }
    return FALSE;
}

gboolean XrandrManager::config_is_all_off (MateRRConfig *config)
{
    int j;
    MateRROutputInfo **outputs;

    outputs = mate_rr_config_get_outputs (config);

    for (j = 0; outputs[j] != NULL; ++j) {
            if (mate_rr_output_info_is_active (outputs[j])) {
                    return FALSE;
            }
    }
    return TRUE;
}

void XrandrManager::print_configuration (MateRRConfig *config, const char *header)
{
    int i;
    MateRROutputInfo **outputs;

    g_print ("=== %s Configuration ===\n", header);
    if (!config) {
            g_print ("  none\n");
            return;
    }

    outputs = mate_rr_config_get_outputs (config);
    for (i = 0; outputs[i] != NULL; ++i)
                print_output (outputs[i]);
}

void XrandrManager::print_output (MateRROutputInfo *info)
{
        int x, y, width, height;

        g_print ("  Output: %s attached to %s\n", mate_rr_output_info_get_display_name (info), mate_rr_output_info_get_name (info));
        g_print ("     status: %s\n", mate_rr_output_info_is_active (info) ? "on" : "off");

        mate_rr_output_info_get_geometry (info, &x, &y, &width, &height);
        g_print ("     width: %d\n", width);
        g_print ("     height: %d\n", height);
        g_print ("     rate: %d\n", mate_rr_output_info_get_refresh_rate (info));
        g_print ("     position: %d %d\n", x, y);
}



gboolean XrandrManager::apply_default_configuration_from_file(XrandrManager *manager, guint32 timestamp)
{
    char *default_config_filename;
    gboolean result;

    default_config_filename = g_settings_get_string (manager->settings, CONF_KEY_DEFAULT_CONFIGURATION_FILE);
    if (!default_config_filename)
            return FALSE;

    result = apply_configuration_from_filename (manager, default_config_filename, TRUE, timestamp, NULL);

    g_free (default_config_filename);
    return result;
}


gboolean XrandrManager::apply_stored_configuration_at_startup(XrandrManager *manager, guint32 timestamp)
{
    GError *my_error;
    gboolean success;
    char *backup_filename;
    char *intended_filename;

    backup_filename = mate_rr_config_get_backup_filename ();
    intended_filename = mate_rr_config_get_intended_filename ();

    /* 1. See if there was a "saved" configuration.  If there is one, it means
     * that the user had selected to change the display configuration, but the
     * machine crashed.  In that case, we'll apply *that* configuration and save it on top of the
     * "intended" one.
     */

    my_error = NULL;

    success = apply_configuration_from_filename (manager, backup_filename, FALSE, timestamp, &my_error);
    if (success) {
            /* The backup configuration existed, and could be applied
             * successfully, so we must restore it on top of the
             * failed/intended one.
             */
            restore_backup_configuration (manager, backup_filename, intended_filename, timestamp);
            goto out;
    }

    if (!g_error_matches (my_error, G_FILE_ERROR, G_FILE_ERROR_NOENT)) {
            /* Epic fail:  there (probably) was a backup configuration, but
             * we could not apply it.  The only thing we can do is delete
             * the backup configuration.  Let's hope that the user doesn't
             * get left with an unusable display...
             */

            unlink (backup_filename);
            goto out;
    }

    /* 2. There was no backup configuration!  This means we are
     * good.  Apply the intended configuration instead.
     */

    success = apply_intended_configuration (manager, intended_filename, timestamp);

out:
    if (my_error)
            g_error_free (my_error);

    g_free (backup_filename);
    g_free (intended_filename);

    return success;
}

gboolean XrandrManager::apply_intended_configuration(XrandrManager *manager,
                                                     const char *intended_filename,
                                                     guint32 timestamp)
{
    GError *my_error;
    gboolean result;

    my_error = NULL;
    result = apply_configuration_from_filename (manager, intended_filename, TRUE, timestamp, &my_error);
    if (!result) {
        if (my_error) {
                if (!g_error_matches (my_error, G_FILE_ERROR, G_FILE_ERROR_NOENT) &&
                    !g_error_matches (my_error, MATE_RR_ERROR, MATE_RR_ERROR_NO_MATCHING_CONFIG))
                        error_message (manager, "Could not apply the stored configuration for monitors", my_error, NULL);/*String Chinese localisation is not handled*/

                g_error_free (my_error);
        }
    }

    return result;
}



void XrandrManager::on_config_changed(GSettings *settings, gchar *key, XrandrManager *manager)
{
    if (g_strcmp0 (key, CONF_KEY_SHOW_NOTIFICATION_ICON) == 0)
                    start_or_stop_icon (manager);
}

void XrandrManager::start_or_stop_icon(XrandrManager *manager)
{
    if (g_settings_get_boolean (manager->settings, CONF_KEY_SHOW_NOTIFICATION_ICON)) {
           status_icon_start (manager);
    }
    else {
           status_icon_stop (manager);
    }
}

void XrandrManager::status_icon_start(XrandrManager *manager)
{
    if (!manager->status_icon) {
            manager->status_icon = gtk_status_icon_new_from_icon_name (USD_XRANDR_ICON_NAME);
            gtk_status_icon_set_tooltip_text (manager->status_icon, "Configure display settings"); /*String Chinese localisation is not handled*/

            g_signal_connect (manager->status_icon, "activate",
                              G_CALLBACK (status_icon_activate_cb), manager);
            g_signal_connect (manager->status_icon, "popup-menu",
                              G_CALLBACK (status_icon_popup_menu_cb), manager);
    }
}

void XrandrManager::status_icon_stop(XrandrManager *manager)
{
    if (manager->status_icon) {
        g_signal_handlers_disconnect_by_func (
                manager->status_icon, (void *)G_CALLBACK (status_icon_activate_cb),manager);

        g_signal_handlers_disconnect_by_func (
                manager->status_icon, (void *)G_CALLBACK (status_icon_popup_menu_cb), manager);

        /* hide the icon before unreffing it; otherwise we will leak
           whitespace in the notification area due to a bug in there */
        gtk_status_icon_set_visible (manager->status_icon, FALSE);
        g_object_unref (manager->status_icon);
        manager->status_icon = NULL;
    }
}

void XrandrManager::status_icon_activate_cb(GtkStatusIcon *status_icon, gpointer data)
{
    XrandrManager *managers = (XrandrManager *)data;
    status_icon_popup_menu(managers,0,gtk_get_current_event_time ());
}
void XrandrManager::status_icon_popup_menu_cb(GtkStatusIcon *status_icon,
                                              unsigned int button,
                                              unsigned int timestamp,
                                              gpointer data)
{
    XrandrManager *managers = (XrandrManager *)data;
    status_icon_popup_menu(managers,button,timestamp);
}



void XrandrManager::on_randr_event(MateRRScreen *screen)
{
    unsigned int change_timestamp, config_timestamp;

    if (! manager->running)
        return;
    mate_rr_screen_get_timestamps(screen, &change_timestamp, &config_timestamp);
    log_open();
    log_msg("Got RANDR event with timestamps change=%u %c config=%u\n",
            change_timestamp,
            timestamp_relationship (change_timestamp, config_timestamp),
            config_timestamp);
    if (change_timestamp > config_timestamp){
        show_timestamps_dialog ("ignoring since change > config");
        log_msg ("  Ignoring event since change >= config\n");
    }else
    {
        char *intended_filename;
        GError *error;
        gboolean success;

        show_timestamps_dialog ( "need to deal with reconfiguration, as config > change");
        intended_filename = mate_rr_config_get_intended_filename ();
        error = NULL;
        success = apply_configuration_from_filename (manager, intended_filename, TRUE, config_timestamp, &error);
        g_free (intended_filename);

        if (!success) {
            if(error)
                g_error_free(error);
            if (config_timestamp != manager->last_config_timestamp){
                manager->last_config_timestamp = config_timestamp;
                auto_configure_outputs (config_timestamp);
                log_msg("  Automatically configured outputs to deal with event\n");
                CT_SYSLOG(LOG_DEBUG,"  Automatically configured outputs to deal with event\n")
            }
            else{
                log_msg ("Applied stored configuration to deal with event\n");
                 CT_SYSLOG(LOG_DEBUG,"Applied stored configuration to deal with event\n");
            }
        }

    }
    /* poke mate-color-manager */
    apply_color_profiles();
    refresh_tray_icon_menu_if_active ( MAX (change_timestamp, config_timestamp));

    log_close ();
}

void XrandrManager::show_timestamps_dialog (const char *msg)
{
#if 1
        return;
#else
        GtkWidget *dialog;
        guint32 change_timestamp, config_timestamp;
        static int serial;

        mate_rr_screen_get_timestamps (msg->rw_screen, &change_timestamp, &config_timestamp);

        dialog = gtk_message_dialog_new (NULL,
                                         0,
                                         GTK_MESSAGE_INFO,
                                         GTK_BUTTONS_CLOSE,
                                         "RANDR timestamps (%d):\n%s\nchange: %u\nconfig: %u",
                                         serial++,
                                         msg,
                                         change_timestamp,
                                         config_timestamp);
        g_signal_connect (dialog, "response",
                          G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_widget_show (dialog);
#endif
}

gboolean XrandrManager::apply_configuration_from_filename(XrandrManager *manager,
                                                          const char *filename,
                                                          gboolean no_matching_config_is_an_error,
                                                          unsigned int timestamp,
                                                          GError **error)
{
    GError *my_error;
    gboolean success;

    char str[512];
    memset(str,0,sizeof (str));
    sprintf(str, "Applying %s with timestamp %d", filename, timestamp);
    show_timestamps_dialog (str);
    my_error = NULL;
    success = mate_rr_config_apply_from_filename_with_time (manager->rw_screen, filename, timestamp, &my_error);
    if (success)
        return TRUE;
    if (g_error_matches (my_error, MATE_RR_ERROR, MATE_RR_ERROR_NO_MATCHING_CONFIG)) {
        if (no_matching_config_is_an_error)
            goto fail;
        g_error_free (my_error);
                        return TRUE;

    }
fail:
    g_propagate_error (error, my_error);
    return FALSE;
}


void XrandrManager::auto_configure_outputs(unsigned int timestamp)
{
    MateRRConfig *config;
    MateRROutputInfo **outputs;
    int i;
    GList *just_turned_on;
    GList *l;
    int x;
    GError *error;
    gboolean applicable;

    config = mate_rr_config_new_current (manager->rw_screen, NULL);
    just_turned_on = NULL;
    outputs = mate_rr_config_get_outputs (config);

    /* For outputs that are connected and on (i.e. they have a CRTC assigned
             * to them, so they are getting a signal), we leave them as they are
             * with their current modes.
             *
             * For other outputs, we will turn on connected-but-off outputs and turn
             * off disconnected-but-on outputs.
             *
             * FIXME: If an output remained connected+on, it would be nice to ensure
             * that the output's CRTCs still has a reasonable mode (think of
             * changing one monitor for another with different capabilities).
    */

    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];

        if (mate_rr_output_info_is_connected (output) && !mate_rr_output_info_is_active (output)) {
            mate_rr_output_info_set_active (output, TRUE);
            mate_rr_output_info_set_rotation (output, MATE_RR_ROTATION_0);
            just_turned_on = g_list_prepend (just_turned_on, GINT_TO_POINTER (i));
        } else if (!mate_rr_output_info_is_connected (output) && mate_rr_output_info_is_active (output))
            mate_rr_output_info_set_active (output, FALSE);
    }

    /* Now, lay out the outputs from left to right.  Put first the outputs
    * which remained on; put last the outputs that were newly turned on.
    */

    x=0;

    /* First, outputs that remained on */

    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];

        if (g_list_find (just_turned_on, GINT_TO_POINTER (i)))
                continue;

        if (mate_rr_output_info_is_active (output)) {
                int width, height;
                g_assert (mate_rr_output_info_is_connected (output));

                mate_rr_output_info_get_geometry (output, NULL, NULL, &width, &height);
                mate_rr_output_info_set_geometry (output, x, 0, width, height);
                x += width;
        }
    }

    /*Second, outputs that were newly-turned on */

    for (l = just_turned_on; l; l = l->next) {
            MateRROutputInfo *output;
            int width;

            i = GPOINTER_TO_INT (l->data);
            output = outputs[i];

            g_assert (mate_rr_output_info_is_active (output) && mate_rr_output_info_is_connected (output));

            /* since the output was off, use its preferred width/height (it doesn't have a real width/height yet) */
            width = mate_rr_output_info_get_preferred_width (output);
            mate_rr_output_info_set_geometry (output, x, 0, width, mate_rr_output_info_get_preferred_height (output));

            x += width;
    }
    /* Check if we have a large enough framebuffer size.  If not, turn off
     * outputs from right to left until we reach a usable size.
    */

    just_turned_on = g_list_reverse (just_turned_on); /* now the outputs here are from right to left */

    l = just_turned_on;
    while (1) {
        MateRROutputInfo *output;
        gboolean is_bounds_error;

        error = NULL;
        applicable = mate_rr_config_applicable (config, manager->rw_screen, &error);

        if (applicable)
                break;

        is_bounds_error = g_error_matches (error, MATE_RR_ERROR, MATE_RR_ERROR_BOUNDS_ERROR);
        g_error_free (error);

        if (!is_bounds_error)
                break;

        if (l) {
                i = GPOINTER_TO_INT (l->data);
                l = l->next;

                output = outputs[i];
                mate_rr_output_info_set_active (output, FALSE);
        } else
                break;
    }
    /* Apply the configuration! */

   if (applicable)
           apply_configuration_and_display_error (manager,config, timestamp);

   g_list_free (just_turned_on);
   g_object_unref (config);

   /* Finally, even though we did a best-effort job in sanitizing the
    * outputs, we don't know the physical layout of the monitors.  We'll
    * start the display capplet so that the user can tweak things to his
    * liking.
    */

#if 0
   /* FIXME: This is disabled for now.  The capplet is not a single-instance application.
    * If you do this:
    *
    *   1. Start the display capplet
    *
    *   2. Plug an extra monitor
    *
    *   3. Hit the "Detect displays" button
    *
    * Then we will get a RANDR event because X re-probes the outputs.  We don't want to
    * start up a second display capplet right there!
    */

   run_display_capplet (NULL);
#endif
}

gboolean XrandrManager::apply_configuration_and_display_error(XrandrManager *manager,
                                                              MateRRConfig *config,
                                                              unsigned int timestamp)
{
    GError *error;
    gboolean success;

    error = NULL;
    success = mate_rr_config_apply_with_time (config, manager->rw_screen, timestamp, &error);
    if (!success) {
        log_msg ("Could not switch to the following configuration (timestamp %u): %s\n", timestamp, error->message);
        log_configuration (config);
        error_message (manager,"Could not switch the monitor configuration", error, NULL);/*String Chinese localisation is not handled*/
        g_error_free (error);
    }

    return success;
}

void XrandrManager::error_message (XrandrManager    *manager,
                                   const char *primary_text,
                                   GError *error_to_display,
                                   const char *secondary_text)
{
#ifdef HAVE_LIBNOTIEY
    NotifyNotification *notification;
    assert (error_to_display == NULL || secondary_text == NULL);

    if(mgr->status_icon)    
        notification = notify_notification_new(primary_text,
                                               error_to_display ? error_to_display->message : secondary_text,
                                               gtk_status_icon_get_icon_name(mgr->status_icon));
    else
        notification = notify_notification_new (primary_text,
                                                error_to_display ? error_to_display->message : secondary_text,
                                                USD_XRANDR_ICON_NAME);
    notify_notification_show (notification, NULL); /* NULL-GError */


#else

    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                     "%s", primary_text);
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
                                              error_to_display ? error_to_display->message : secondary_text);

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
#endif
}


void XrandrManager::apply_color_profiles()
{
    gboolean ret;
    GError *error = NULL;

    /* run the mate-color-manager apply program */
    ret = g_spawn_command_line_async ("/usr/bin/gcm-apply", &error);
    if (!ret){
        /* only print the warning if the binary is installed */
       if (error->code != G_SPAWN_ERROR_NOENT) {
            g_warning ("failed to apply color profiles: %s", error->message);
            CT_SYSLOG(LOG_DEBUG,"failed to apply color profiles: %s", error->message);
       }
       g_error_free (error);
    }

}


void XrandrManager::refresh_tray_icon_menu_if_active( unsigned int timestamp)
{
    if (manager->popup_menu) {
        gtk_menu_shell_cancel (GTK_MENU_SHELL (manager->popup_menu)); /* status_icon_popup_menu_selection_done_cb() will free everything */
        status_icon_popup_menu (manager,0, timestamp);
    }
}


void XrandrManager::status_icon_popup_menu(XrandrManager *manager,unsigned int button, unsigned int timestamp)
{
    GtkWidget *item;

            g_assert (manager->configuration == NULL);
            manager->configuration = mate_rr_config_new_current (manager->rw_screen, NULL);

            g_assert (manager->labeler == NULL);
            manager->labeler = mate_rr_labeler_new (manager->configuration);

            g_assert (manager->popup_menu == NULL);
            manager->popup_menu = gtk_menu_new ();

            add_menu_items_for_outputs ();

            item = gtk_separator_menu_item_new ();
            gtk_widget_show (item);
            gtk_menu_shell_append (GTK_MENU_SHELL (manager->popup_menu), item);

            item = gtk_menu_item_new_with_mnemonic ("_Configure Display Settings…");/*String Chinese localisation is not handled*/
            g_signal_connect (item, "activate",
                              G_CALLBACK (popup_menu_configure_display_cb), manager);
            gtk_widget_show (item);
            gtk_menu_shell_append (GTK_MENU_SHELL (manager->popup_menu), item);

            g_signal_connect (manager->popup_menu, "selection-done",
                              G_CALLBACK (status_icon_popup_menu_selection_done_cb), manager);

            /*Set up custom theming and forced transparency support*/
            GtkWidget *toplevel = gtk_widget_get_toplevel (manager->popup_menu);
            /*Fix any failures of compiz/other wm's to communicate with gtk for transparency */
            GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(toplevel));
            GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
            gtk_widget_set_visual(GTK_WIDGET(toplevel), visual);
            /*Set up the gtk theme class from ukui-panel*/
            GtkStyleContext *context;
            context = gtk_widget_get_style_context (GTK_WIDGET(toplevel));
            gtk_style_context_add_class(context,"gnome-panel-menu-bar");
            gtk_style_context_add_class(context,"ukui-panel-menu-bar");

            gtk_menu_popup (GTK_MENU (manager->popup_menu), NULL, NULL,
                            gtk_status_icon_position_menu,
                            manager->status_icon, button, timestamp);

}

void XrandrManager::popup_menu_configure_display_cb(GtkMenuItem *item,gpointer data)
{
    run_display_capplet (GTK_WIDGET (item));
}

void XrandrManager::status_icon_popup_menu_selection_done_cb(GtkMenuShell *menu_shell,gpointer data)
{
    XrandrManager *manager = (XrandrManager *)data;

    gtk_widget_destroy (manager->popup_menu);
    manager->popup_menu = NULL;

    mate_rr_labeler_hide (manager->labeler);
    g_object_unref (manager->labeler);
    manager->labeler = NULL;

    g_object_unref (manager->configuration);
    manager->configuration = NULL;
}

void XrandrManager::add_menu_items_for_outputs()
{
    int i;
    MateRROutputInfo **outputs;
    outputs = mate_rr_config_get_outputs (manager->configuration);
    for (i = 0; outputs[i] != NULL; i++) {
        if (mate_rr_output_info_is_connected (outputs[i]))
                add_menu_items_for_output (outputs[i]);
    }
}

void XrandrManager::add_menu_items_for_output(MateRROutputInfo *output)
{
    GtkWidget *item;

    item = make_menu_item_for_output_title (output);
    gtk_menu_shell_append (GTK_MENU_SHELL (manager->popup_menu), item);

    add_rotation_items_for_output (output);
}


#define OUTPUT_TITLE_ITEM_BORDER 2
#define OUTPUT_TITLE_ITEM_PADDING 4

GtkWidget * XrandrManager::make_menu_item_for_output_title (MateRROutputInfo *output)
{
        GtkWidget *item;
        GtkWidget *label;
        char *str;
        GdkColor black = { 0, 0, 0, 0 };

        item = gtk_menu_item_new ();

        g_signal_connect (item, "size-allocate",
                          G_CALLBACK (title_item_size_allocate_cb), NULL);

        str = g_markup_printf_escaped ("<b>%s</b>", mate_rr_output_info_get_display_name (output));
        label = gtk_label_new (NULL);
        gtk_label_set_markup (GTK_LABEL (label), str);
        g_free (str);

        /* Make the label explicitly black.  We don't want it to follow the
         * theme's colors, since the label is always shown against a light
         * pastel background.  See bgo#556050
         */
        gtk_widget_modify_fg (label, gtk_widget_get_state (label), &black);

        /* Add padding around the label to fit the box that we'll draw for color-coding */
#if GTK_CHECK_VERSION (3, 16, 0)
        gtk_label_set_xalign (GTK_LABEL (label), 0.0);
        gtk_label_set_yalign (GTK_LABEL (label), 0.5);
#else
        gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
#endif
        gtk_misc_set_padding (GTK_MISC (label),
                              OUTPUT_TITLE_ITEM_BORDER + OUTPUT_TITLE_ITEM_PADDING,
                              OUTPUT_TITLE_ITEM_BORDER + OUTPUT_TITLE_ITEM_PADDING);

        gtk_container_add (GTK_CONTAINER (item), label);

        /* We want to paint a colored box as the background of the label, so we connect
         * to its expose-event signal.  See the comment in *** to see why need to connect
         * to the label both 'before' and 'after'.
         */
        g_signal_connect (label, "draw",
                          G_CALLBACK (output_title_label_draw_cb), manager);
        g_signal_connect_after (label, "draw",
                                G_CALLBACK (output_title_label_after_draw_cb) , manager);

        g_object_set_data (G_OBJECT (label), "output", output);

        gtk_widget_set_sensitive (item, FALSE); /* the title is not selectable */
        gtk_widget_show_all (item);

        return item;
}

void XrandrManager::title_item_size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation)
{
        /* When GtkMenu does size_request on its items, it asks them for their "toggle size",
         * which will be non-zero when there are check/radio items.  GtkMenu remembers
         * the largest of those sizes.  During the size_allocate pass, GtkMenu calls
         * gtk_menu_item_toggle_size_allocate() with that value, to tell the menu item
         * that it should later paint its child a bit to the right of its edge.
         *
         * However, we want the "title" menu items for each RANDR output to span the *whole*
         * allocation of the menu item, not just the "allocation minus toggle" area.
         *
         * So, we let the menu item size_allocate itself as usual, but this
         * callback gets run afterward.  Here we hack a toggle size of 0 into
         * the menu item, and size_allocate it by hand *again*.  We also need to
         * avoid recursing into this function.
         */

        g_assert (GTK_IS_MENU_ITEM (widget));

        gtk_menu_item_toggle_size_allocate (GTK_MENU_ITEM (widget), 0);

        g_signal_handlers_block_by_func (widget, (void*)title_item_size_allocate_cb, NULL);

        /* Sigh. There is no way to turn on GTK_ALLOC_NEEDED outside of GTK+
         * itself; also, since calling size_allocate on a widget with the same
         * allcation is a no-op, we need to allocate with a "different" size
         * first.
         */

        allocation->width++;
        gtk_widget_size_allocate (widget, allocation);

        allocation->width--;
        gtk_widget_size_allocate (widget, allocation);

        g_signal_handlers_unblock_by_func (widget, (void*)title_item_size_allocate_cb, NULL);

}

gboolean XrandrManager::output_title_label_draw_cb (GtkWidget *widget, cairo_t *cr)
{
        MateRROutputInfo *output;
        GdkRGBA color;
        GtkAllocation allocation;
        XrandrManager * manager;
        assert (GTK_IS_LABEL (widget));
        const char * str1= "output";

        output = (MateRROutputInfo *)g_object_get_data(G_OBJECT (widget), str1);
        assert (output != NULL);

        assert (manager->labeler != NULL);

        /* Draw a black rectangular border, filled with the color that corresponds to this output */
        mate_rr_labeler_get_rgba_for_output (manager->labeler, output, &color);

        cairo_set_source_rgb (cr, 0, 0, 0);
        cairo_set_line_width (cr, OUTPUT_TITLE_ITEM_BORDER);
        gtk_widget_get_allocation (widget, &allocation);
        cairo_rectangle (cr,
                         allocation.x + OUTPUT_TITLE_ITEM_BORDER / 2.0,
                         allocation.y + OUTPUT_TITLE_ITEM_BORDER / 2.0,
                         allocation.width - OUTPUT_TITLE_ITEM_BORDER,
                         allocation.height - OUTPUT_TITLE_ITEM_BORDER);
        cairo_stroke (cr);

        gdk_cairo_set_source_rgba (cr, &color);
        cairo_rectangle (cr,
                         allocation.x + OUTPUT_TITLE_ITEM_BORDER,
                         allocation.y + OUTPUT_TITLE_ITEM_BORDER,
                         allocation.width - 2 * OUTPUT_TITLE_ITEM_BORDER,
                         allocation.height - 2 * OUTPUT_TITLE_ITEM_BORDER);

        cairo_fill (cr);

        /* We want the label to always show up as if it were sensitive
         * ("style->fg[GTK_STATE_NORMAL]"), even though the label is insensitive
         * due to being inside an insensitive menu item.  So, here we have a
         * HACK in which we frob the label's state directly.  GtkLabel's expose
         * handler will be run after this function, so it will think that the
         * label is in GTK_STATE_NORMAL.  We reset the label's state back to
         * insensitive in output_title_label_after_expose_event_cb().
         *
         * Yay for fucking with GTK+'s internals.
         */

        gtk_widget_set_state (widget, GTK_STATE_NORMAL);

        return FALSE;
}

/* See the comment in output_title_event_box_expose_event_cb() about this funny label widget */
gboolean XrandrManager::output_title_label_after_draw_cb (GtkWidget *widget, cairo_t *cr)
{
        g_assert (GTK_IS_LABEL (widget));
        gtk_widget_set_state (widget, GTK_STATE_INSENSITIVE);

        return FALSE;
}

void XrandrManager::add_rotation_items_for_output (MateRROutputInfo *output)
{

        int num_rotations;
        MateRRRotation rotations;

        get_allowed_rotations_for_output (manager->configuration, manager->rw_screen, output, &num_rotations, &rotations);

        if (num_rotations == 1)
                add_unsupported_rotation_item ();
        else
                add_items_for_rotations (output, rotations);

}

void XrandrManager::get_allowed_rotations_for_output(MateRRConfig *config,
                                                     MateRRScreen *rr_screen,
                                                     MateRROutputInfo *output,
                                                     int *out_num_rotations,
                                                     MateRRRotation *out_rotations)
{
    MateRRRotation current_rotation;
    int i;

    *out_num_rotations = 0;
    *out_rotations = MateRRRotation(0);

    current_rotation = mate_rr_output_info_get_rotation (output);

    /* Yay for brute force */

    for (i = 0; i < G_N_ELEMENTS (possible_rotations); i++) {
            MateRRRotation rotation_to_test;

            rotation_to_test = possible_rotations[i];

            mate_rr_output_info_set_rotation (output, rotation_to_test);

            if (mate_rr_config_applicable (config, rr_screen, NULL)) { /* NULL-GError */
                    (*out_num_rotations)++;
                 (*out_rotations) = MateRRRotation((*out_rotations) | rotation_to_test);
            }
    }

    mate_rr_output_info_set_rotation (output, current_rotation);

    if (*out_num_rotations == 0 || *out_rotations == 0) {
            g_warning ("Huh, output %p says it doesn't support any rotations, and yet it has a current rotation?", output);
            *out_num_rotations = 1;
            *out_rotations = mate_rr_output_info_get_rotation (output);
    }
}

void XrandrManager::add_unsupported_rotation_item ()
{
        GtkWidget *item;
        GtkWidget *label;
        gchar *markup;

        item = gtk_menu_item_new ();

        label = gtk_label_new (NULL);
        markup = g_strdup_printf ("<i>%s</i>", "Rotation not supported");/*String Chinese localisation is not handled*/
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_container_add (GTK_CONTAINER (item), label);

        gtk_widget_show_all (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (manager->popup_menu), item);

}

void XrandrManager::add_items_for_rotations(MateRROutputInfo *output,
                                            MateRRRotation allowed_rotations)
{

    typedef struct {
            MateRRRotation	rotation;
            const char *	name;
    } RotationInfo;
    static const RotationInfo rotations[] = {
            { MATE_RR_ROTATION_0,("Normal") },          /*String Chinese localisation is not handled*/
            { MATE_RR_ROTATION_90, ("Left") },          /*String Chinese localisation is not handled*/
            { MATE_RR_ROTATION_270, ("Right") },        /*String Chinese localisation is not handled*/
            { MATE_RR_ROTATION_180, ("Upside Down") },  /*String Chinese localisation is not handled*/
            /* We don't allow REFLECT_X or REFLECT_Y for now, as mate-display-properties doesn't allow them, either */
    };

    int i;
    GSList *group;
    GtkWidget *active_item;
    gulong active_item_activate_id;

    group = NULL;
    active_item = NULL;
    active_item_activate_id = 0;

    for (i = 0; i < G_N_ELEMENTS (rotations); i++) {
            MateRRRotation rot;
            GtkWidget *item;
            gulong activate_id;

            rot = rotations[i].rotation;

            if ((allowed_rotations & rot) == 0) {
                    /* don't display items for rotations which are
                     * unavailable.  Their availability is not under the
                     * user's control, anyway.
                     */
                    continue;
            }

            item = gtk_radio_menu_item_new_with_label (group, rotations[i].name);/*rotations[i].name Chinese localisation is not handled*/
            gtk_widget_show_all (item);
            gtk_menu_shell_append (GTK_MENU_SHELL (manager->popup_menu), item);

            g_object_set_data (G_OBJECT (item), "output", output);
            g_object_set_data (G_OBJECT (item), "rotation", GINT_TO_POINTER (rot));

            activate_id = g_signal_connect (item, "activate",
                                            G_CALLBACK (output_rotation_item_activate_cb), manager);

            group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));

            if (rot == mate_rr_output_info_get_rotation (output)) {
                    active_item = item;
                    active_item_activate_id = activate_id;
            }
    }

    if (active_item) {
            /* Block the signal temporarily so our callback won't be called;
             * we are just setting up the UI.
             */
            g_signal_handler_block (active_item, active_item_activate_id);

            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (active_item), TRUE);

            g_signal_handler_unblock (active_item, active_item_activate_id);
    }

}
void XrandrManager::output_rotation_item_activate_cb(GtkCheckMenuItem *item)
{

    MateRROutputInfo *output;
    MateRRRotation rotation;
    GError *error;

    /* Not interested in deselected items */
    if (!gtk_check_menu_item_get_active (item))
            return;

    ensure_current_configuration_is_saved ();
    const char *str1 = "output";
    const char *str2 = "rotation";
    output = (MateRROutputInfo *)g_object_get_data (G_OBJECT (item), str1);
    rotation = MateRRRotation(GPOINTER_TO_INT ((MateRROutputInfo *)g_object_get_data (G_OBJECT (item), str2)));

    mate_rr_output_info_set_rotation (output, rotation);

    error = NULL;
    if (!mate_rr_config_save (manager->configuration, &error)) {
            error_message (manager,"Could not save monitor configuration", error, NULL);/*String Chinese localisation is not handled*/
            if (error)
                    g_error_free (error);

            return;
    }

    try_to_apply_intended_configuration (NULL, gtk_get_current_event_time (), NULL); /* NULL-GError */

}

void XrandrManager::ensure_current_configuration_is_saved()
{
    MateRRScreen *rr_screen;
    MateRRConfig *rr_config;

    /* Normally, mate_rr_config_save() creates a backup file based on the
     * old monitors.xml.  However, if *that* file didn't exist, there is
     * nothing from which to create a backup.  So, here we'll save the
     * current/unchanged configuration and then let our caller call
     * mate_rr_config_save() again with the new/changed configuration, so
     * that there *will* be a backup file in the end.
     */

    rr_screen = mate_rr_screen_new (gdk_screen_get_default (), NULL); /* NULL-GError */
    if (!rr_screen)
            return;

    rr_config = mate_rr_config_new_current (rr_screen, NULL);
    mate_rr_config_save (rr_config, NULL); /* NULL-GError */

    g_object_unref (rr_config);
    g_object_unref (rr_screen);
}

gboolean XrandrManager::try_to_apply_intended_configuration(GdkWindow *parent_window, guint32 timestamp, GError **error)
{
    char *backup_filename;
    char *intended_filename;
    gboolean result;

    /* Try to apply the intended configuration */

    backup_filename = mate_rr_config_get_backup_filename ();
    intended_filename = mate_rr_config_get_intended_filename ();

    result = apply_configuration_from_filename (manager,intended_filename, FALSE, timestamp, error);
    if (!result) {
            error_message (manager,"The selected configuration for displays could not be applied", error ? *error : NULL, NULL);/*String Chinese localisation is not handled*/
            restore_backup_configuration_without_messages (backup_filename, intended_filename);
            goto out;
    } else {
            /* We need to return as quickly as possible, so instead of
             * confirming with the user right here, we do it in an idle
             * handler.  The caller only expects a status for "could you
             * change the RANDR configuration?", not "is the user OK with it
             * as well?".
             */
            queue_confirmation_by_user (parent_window, timestamp);
    }

    out:
        g_free (backup_filename);
        g_free (intended_filename);
        return result;
}

void XrandrManager::restore_backup_configuration_without_messages (const char *backup_filename,
                                                           const char *intended_filename)
{
        backup_filename = mate_rr_config_get_backup_filename ();
        rename (backup_filename, intended_filename);
}

void XrandrManager::queue_confirmation_by_user (GdkWindow *parent_window, guint32 timestamp)
{
        struct confirmation *confirmation;

        confirmation = g_new (struct confirmation, 1);
        confirmation->managers = manager;
        confirmation->parent_window = parent_window;
        confirmation->timestamp = timestamp;

        g_idle_add (confirm_with_user_idle_cb, confirmation);
}

gboolean XrandrManager::confirm_with_user_idle_cb (gpointer data)
{
    struct confirmation *confirmation =(struct confirmation *)data ;

    char *backup_filename;
    char *intended_filename;

    static int stat = 0;
    if (stat)
        return FALSE;
    stat = 1;

    backup_filename = mate_rr_config_get_backup_filename ();
    intended_filename = mate_rr_config_get_intended_filename ();

    if (user_says_things_are_ok (confirmation->managers, confirmation->parent_window))
            unlink (backup_filename);
    else
            restore_backup_configuration (confirmation->managers, backup_filename, intended_filename, confirmation->timestamp);

    g_free (confirmation);
    stat = 0;

    return FALSE;
}


#define CONFIRMATION_DIALOG_SECONDS 30
gboolean XrandrManager::user_says_things_are_ok (XrandrManager *managers, GdkWindow *parent_window)
{
    TimeoutDialog timeout;
    guint timeout_id;

    timeout.managers = managers;

    timeout.dialog = gtk_message_dialog_new (NULL,
                                             GTK_DIALOG_MODAL,
                                             GTK_MESSAGE_QUESTION,
                                             GTK_BUTTONS_NONE,
                                             "Does the display look OK?");/*String Chinese localisation is not handled*/

    timeout.countdown = CONFIRMATION_DIALOG_SECONDS;

    print_countdown_text (&timeout);

    gtk_window_set_icon_name (GTK_WINDOW (timeout.dialog), "preferences-desktop-display");
    gtk_dialog_add_button (GTK_DIALOG (timeout.dialog), "_Restore Previous Configuration", GTK_RESPONSE_CANCEL);/*String Chinese localisation is not handled*/
    gtk_dialog_add_button (GTK_DIALOG (timeout.dialog), "_Keep This Configuration", GTK_RESPONSE_ACCEPT);  /*String Chinese localisation is not handled*/
    gtk_dialog_set_default_response (GTK_DIALOG (timeout.dialog), GTK_RESPONSE_ACCEPT); /* ah, the optimism */

    g_signal_connect (timeout.dialog, "response",
                      G_CALLBACK (timeout_response_cb),
                      &timeout);

    gtk_widget_realize (timeout.dialog);

    if (parent_window)
            gdk_window_set_transient_for (gtk_widget_get_window (timeout.dialog), parent_window);

    gtk_widget_show_all (timeout.dialog);
    /* We don't use g_timeout_add_seconds() since we actually care that the user sees "real" second ticks in the dialog */
    timeout_id = g_timeout_add (1000,
                                timeout_cb,
                                &timeout);
    gtk_main ();

    gtk_widget_destroy (timeout.dialog);
    g_source_remove (timeout_id);

    if (timeout.response_id == GTK_RESPONSE_ACCEPT)
            return TRUE;
    else
            return FALSE;
}


void XrandrManager::print_countdown_text (TimeoutDialog *timeout)
{
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (timeout->dialog),
                                                  ngettext ("The display will be reset to its previous configuration in %d second",
                                                            "The display will be reset to its previous configuration in %d seconds",
                                                            timeout->countdown),
                                                  timeout->countdown);
}

void XrandrManager::timeout_response_cb (GtkDialog *dialog, int response_id, gpointer data)
{
        TimeoutDialog *timeout = (TimeoutDialog *)data;

        if (response_id == GTK_RESPONSE_DELETE_EVENT) {
                /* The user closed the dialog or pressed ESC, revert */
                timeout->response_id = GTK_RESPONSE_CANCEL;
        } else
                timeout->response_id = response_id;

        gtk_main_quit ();
}

gboolean XrandrManager::timeout_cb (gpointer data)
{
        TimeoutDialog *timeout = (TimeoutDialog *)data;

        timeout->countdown--;

        if (timeout->countdown == 0) {
                timeout->response_id = GTK_RESPONSE_CANCEL;
                gtk_main_quit ();
        } else {
                print_countdown_text (timeout);
        }

        return TRUE;
}


void XrandrManager::restore_backup_configuration (XrandrManager *manager,
                                                  const char *backup_filename,
                                                  const char *intended_filename,
                                                  unsigned int timestamp)
{
    int saved_errno;
    if (rename (backup_filename, intended_filename) == 0) {
        GError *error;

        error = NULL;
        if (!apply_configuration_from_filename (manager, intended_filename, FALSE, timestamp, &error)) {
            error_message (manager, "Could not restore the display's configuration", error, NULL); /*String Chinese localisation is not handled*/

            if (error)
                    g_error_free (error);
        }

            return;
    }

    saved_errno = errno;

    /* ENOENT means the original file didn't exist.  That is *not* an error;
     * the backup was not created because there wasn't even an original
     * monitors.xml (such as on a first-time login).  Note that *here* there
     * is a "didn't work" monitors.xml, so we must delete that one.
     */
    if (saved_errno == ENOENT)
            unlink (intended_filename);
    else {
            char *msg;

            msg = g_strdup_printf ("Could not rename %s to %s: %s",
                                   backup_filename, intended_filename,
                                   g_strerror (saved_errno));
            error_message (manager,
                           "Could not restore the display's configuration from a backup",/*String Chinese localisation is not handled*/
                           NULL,
                           msg);
            g_free (msg);
    }

    unlink (backup_filename);
}
