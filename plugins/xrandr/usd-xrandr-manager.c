/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2007, 2008 Red Hat, Inc
 * Copyright (C) 2017 Tianjin KYLIN Information Technology Co., Ltd.
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
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <dlfcn.h>

#include <locale.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <dbus/dbus-glib.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/Xrandr.h>
#include <xorg/xserver-properties.h>
#include <gudev/gudev.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-rr-config.h>
#include <libmate-desktop/mate-rr.h>
#include <libmate-desktop/mate-rr-labeler.h>
#include <libmate-desktop/mate-desktop-utils.h>

#ifdef HAVE_LIBNOTIFY
#include <libnotify/notify.h>
#endif

#include "ukui-settings-profile.h"
#include "usd-xrandr-manager.h"

#define USD_XRANDR_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), USD_TYPE_XRANDR_MANAGER, UsdXrandrManagerPrivate))

#define CONF_SCHEMA                                    "org.ukui.SettingsDaemon.plugins.xrandr"
#define CONF_KEY_SHOW_NOTIFICATION_ICON                "show-notification-icon"
#define CONF_KEY_USE_XORG_MONITOR_SETTINGS             "use-xorg-monitor-settings"
#define CONF_KEY_TURN_ON_EXTERNAL_MONITORS_AT_STARTUP  "turn-on-external-monitors-at-startup"
#define CONF_KEY_TURN_ON_LAPTOP_MONITOR_AT_STARTUP     "turn-on-laptop-monitor-at-startup"
#define CONF_KEY_DEFAULT_CONFIGURATION_FILE            "default-configuration-file"
#define CONF_KEY_XRANDR_WIN_SHOW                       "xrandr-apply"

#define XSETTINGS_SCHEMA                               "org.ukui.SettingsDaemon.plugins.xsettings"
#define XSETTINGS_KEY_SCALING                          "scaling-factor"

#define VIDEO_KEYSYM    "XF86Display"
#define ROTATE_KEYSYM   "XF86RotateWindows"

/* Number of seconds that the confirmation dialog will last before it resets the
 * RANDR configuration to its old state.
 */
#define CONFIRMATION_DIALOG_SECONDS 15

/* name of the icon files (usd-xrandr.svg, etc.) */
#define USD_XRANDR_ICON_NAME "uksd-xrandr"

/* executable of the control center's display configuration capplet */
#define USD_XRANDR_DISPLAY_CAPPLET "mate-display-properties"

#define USD_DBUS_PATH "/org/ukui/SettingsDaemon"
#define USD_DBUS_NAME "org.ukui.SettingsDaemon"
#define USD_XRANDR_DBUS_PATH USD_DBUS_PATH "/XRANDR"
#define USD_XRANDR_DBUS_NAME USD_DBUS_NAME ".XRANDR"

#define MAX_SIZE_MATCH_DIFF 0.05

typedef struct
{
    unsigned char *input_node;
    XIDeviceInfo dev_info;

}TsInfo;

typedef struct
{
    int touchId;               //触摸屏ID
    char cMonitorName[64];     //显示器名称
}TouchMapInfo;                 //触摸屏和显示器映射关系信息

GList *g_TouchMapList = NULL;  //触摸屏和显示器映射关系记录

enum{
    noshow,
    show,
    certain,
    cancel,
};

struct UsdXrandrManagerPrivate
{
        DBusGConnection *dbus_connection;

        /* Key code of the XF86Display key (Fn-F7 on Thinkpads, Fn-F4 on HP machines, etc.) */
        guint switch_video_mode_keycode;

        /* Key code of the XF86RotateWindows key (present on some tablets) */
        guint rotate_windows_keycode;

        MateRRScreen *rw_screen;
        gboolean running;

        GtkStatusIcon *status_icon;
        GtkWidget *popup_menu;
        MateRRConfig *configuration;
        MateRRLabeler *labeler;
        GSettings *settings;

        /* fn-F7 status */
        int             current_fn_f7_config;             /* -1 if no configs */
        MateRRConfig **fn_f7_configs;  /* NULL terminated, NULL if there are no configs */

        /* Last time at which we got a "screen got reconfigured" event; see on_randr_event() */
        guint32 last_config_timestamp;
};

static const MateRRRotation possible_rotations[] = {
        MATE_RR_ROTATION_0,
        MATE_RR_ROTATION_90,
        MATE_RR_ROTATION_180,
        MATE_RR_ROTATION_270
        /* We don't allow REFLECT_X or REFLECT_Y for now, as mate-display-properties doesn't allow them, either */
};

static void     usd_xrandr_manager_class_init  (UsdXrandrManagerClass *klass);
static void     usd_xrandr_manager_init        (UsdXrandrManager      *xrandr_manager);
static void     usd_xrandr_manager_finalize    (GObject             *object);

static void error_message (UsdXrandrManager *mgr, const char *primary_text, GError *error_to_display, const char *secondary_text);

static void status_icon_popup_menu (UsdXrandrManager *manager, guint button, guint32 timestamp);
static void run_display_capplet (GtkWidget *widget);
static void get_allowed_rotations_for_output (MateRRConfig *config,
                                              MateRRScreen *rr_screen,
                                              MateRROutputInfo *output,
                                              int *out_num_rotations,
                                              MateRRRotation *out_rotations);

G_DEFINE_TYPE (UsdXrandrManager, usd_xrandr_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static FILE *log_file;

static void
log_open (void)
{
        char *toggle_filename;
        char *log_filename;
        struct stat st;

        if (log_file)
                return;

        toggle_filename = g_build_filename (g_get_home_dir (), "usd-debug-randr", NULL);
        log_filename = g_build_filename (g_get_home_dir (), "usd-debug-randr.log", NULL);

        if (stat (toggle_filename, &st) != 0)
                goto out;

        log_file = fopen (log_filename, "a");

        if (log_file && ftell (log_file) == 0)
                fprintf (log_file, "To keep this log from being created, please rm ~/usd-debug-randr\n");

out:
        g_free (toggle_filename);
        g_free (log_filename);
}

static void
log_close (void)
{
        if (log_file) {
                fclose (log_file);
                log_file = NULL;
        }
}

static void
log_msg (const char *format, ...)
{
        if (log_file) {
                va_list args;

                va_start (args, format);
                vfprintf (log_file, format, args);
                va_end (args);
        }
}

static void
log_output (MateRROutputInfo *output)
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

static void
log_configuration (MateRRConfig *config)
{
        int i;
        MateRROutputInfo **outputs = mate_rr_config_get_outputs (config);

        log_msg ("        cloned: %s\n", mate_rr_config_get_clone (config) ? "yes" : "no");

        for (i = 0; outputs[i] != NULL; i++)
                log_output (outputs[i]);

        if (i == 0)
                log_msg ("        no outputs!\n");
}

static char
timestamp_relationship (guint32 a, guint32 b)
{
        if (a < b)
                return '<';
        else if (a > b)
                return '>';
        else
                return '=';
}

static void
log_screen (MateRRScreen *screen)
{
        MateRRConfig *config;
        int min_w, min_h, max_w, max_h;
        guint32 change_timestamp, config_timestamp;

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

static void
log_configurations (MateRRConfig **configs)
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

static void
show_timestamps_dialog (UsdXrandrManager *manager, const char *msg)
{
#if 1
        return;
#else
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *dialog;
        guint32 change_timestamp, config_timestamp;
        static int serial;

        mate_rr_screen_get_timestamps (priv->rw_screen, &change_timestamp, &config_timestamp);

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

/* This function centralizes the use of mate_rr_config_apply_from_filename_with_time().
 *
 * Optionally filters out MATE_RR_ERROR_NO_MATCHING_CONFIG from
 * mate_rr_config_apply_from_filename_with_time(), since that is not usually an error.
 */
static gboolean
apply_configuration_from_filename (UsdXrandrManager *manager,
                                   const char       *filename,
                                   gboolean          no_matching_config_is_an_error,
                                   guint32           timestamp,
                                   GError          **error)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        GError *my_error;
        gboolean success;
        char *str;

        str = g_strdup_printf ("Applying %s with timestamp %d", filename, timestamp);
        show_timestamps_dialog (manager, str);
        g_free (str);

        my_error = NULL;
        success = mate_rr_config_apply_from_filename_with_time (priv->rw_screen, filename, timestamp, &my_error);
        if (success)
                return TRUE;

        if (g_error_matches (my_error, MATE_RR_ERROR, MATE_RR_ERROR_NO_MATCHING_CONFIG)) {
                if (no_matching_config_is_an_error)
                        goto fail;

                /* This is not an error; the user probably changed his monitors
                 * and so they don't match any of the stored configurations.
                 */
                g_error_free (my_error);
                return TRUE;
        }

fail:
        g_propagate_error (error, my_error);
        return FALSE;
}

/* This function centralizes the use of mate_rr_config_apply_with_time().
 *
 * Applies a configuration and displays an error message if an error happens.
 * We just return whether setting the configuration succeeded.
 */
static gboolean
apply_configuration_and_display_error (UsdXrandrManager *manager, MateRRConfig *config, guint32 timestamp)
{
        UsdXrandrManagerPrivate *priv = manager->priv;
        GError *error;
        gboolean success;

        error = NULL;
        success = mate_rr_config_apply_with_time (config, priv->rw_screen, timestamp, &error);
        if (!success) {
                log_msg ("Could not switch to the following configuration (timestamp %u): %s\n", timestamp, error->message);
                log_configuration (config);
                error_message (manager, _("Could not switch the monitor configuration"), error, NULL);
                g_error_free (error);
        }

        return success;
}

static void
restore_backup_configuration_without_messages (const char *backup_filename, const char *intended_filename)
{
        backup_filename = mate_rr_config_get_backup_filename ();
        rename (backup_filename, intended_filename);
}

static void
restore_backup_configuration (UsdXrandrManager *manager, const char *backup_filename, const char *intended_filename, guint32 timestamp)
{
        int saved_errno;

        if (rename (backup_filename, intended_filename) == 0) {
                GError *error;

                error = NULL;
                if (!apply_configuration_from_filename (manager, intended_filename, FALSE, timestamp, &error)) {
                        error_message (manager, _("Could not restore the display's configuration"), error, NULL);

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
                               _("Could not restore the display's configuration from a backup"),
                               NULL,
                               msg);
                g_free (msg);
        }

        unlink (backup_filename);
}

typedef struct {
        UsdXrandrManager *manager;
        GtkWidget *dialog;

        int countdown;
        int response_id;
} TimeoutDialog;

static void
print_countdown_text (TimeoutDialog *timeout)
{
        gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (timeout->dialog),
                                                  ngettext ("The display will be reset to its previous configuration in %d second",
                                                            "The display will be reset to its previous configuration in %d seconds",
                                                            timeout->countdown),
                                                  timeout->countdown);
}

static gboolean
timeout_cb (gpointer data)
{
        TimeoutDialog *timeout = data;

        timeout->countdown--;

        if (timeout->countdown == 0) {
                timeout->response_id = GTK_RESPONSE_CANCEL;
                gtk_main_quit ();
        } else {
                print_countdown_text (timeout);
        }

        return TRUE;
}

static void
timeout_response_cb (GtkDialog *dialog, int response_id, gpointer data)
{
        TimeoutDialog *timeout = data;

        if (response_id == GTK_RESPONSE_DELETE_EVENT) {
                /* The user closed the dialog or pressed ESC, revert */
                timeout->response_id = GTK_RESPONSE_CANCEL;
        } else
                timeout->response_id = response_id;

        gtk_main_quit ();
}

static gboolean
user_says_things_are_ok (UsdXrandrManager *manager, GdkWindow *parent_window)
{
    //设置GSettings，表示计时窗口弹出
    UsdXrandrManagerPrivate *priv = manager->priv;
    // g_settings_set_boolean(priv->settings, CONF_KEY_XRANDR_WIN_SHOW, TRUE);
    g_settings_set_enum(priv->settings, CONF_KEY_XRANDR_WIN_SHOW, show);
        TimeoutDialog timeout;
        guint timeout_id;

        timeout.manager = manager;

        timeout.dialog = gtk_message_dialog_new (NULL,
                                                 GTK_DIALOG_MODAL,
                                                 GTK_MESSAGE_QUESTION,
                                                 GTK_BUTTONS_NONE,
                                                 _("Does the display look OK?"));

        timeout.countdown = CONFIRMATION_DIALOG_SECONDS;

        print_countdown_text (&timeout);

        gtk_window_set_icon_name (GTK_WINDOW (timeout.dialog), "preferences-desktop-display");
        gtk_dialog_add_button (GTK_DIALOG (timeout.dialog), _("_Restore Previous Configuration"), GTK_RESPONSE_CANCEL);
        gtk_dialog_add_button (GTK_DIALOG (timeout.dialog), _("_Keep This Configuration"), GTK_RESPONSE_ACCEPT);
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

        if (timeout.response_id == GTK_RESPONSE_ACCEPT) {
                // g_settings_set_boolean(priv->settings, CONF_KEY_XRANDR_WIN_SHOW, FALSE);
                g_settings_set_enum(priv->settings, CONF_KEY_XRANDR_WIN_SHOW, show);
                return TRUE;
		}
        else {
		         // g_settings_set_boolean(priv->settings, CONF_KEY_XRANDR_WIN_SHOW, FALSE);
                 g_settings_set_enum(priv->settings, CONF_KEY_XRANDR_WIN_SHOW, show);
                return FALSE;
		}
}

struct confirmation {
        UsdXrandrManager *manager;
        GdkWindow *parent_window;
        guint32 timestamp;
};

static gboolean
confirm_with_user_idle_cb (gpointer data)
{
        struct confirmation *confirmation = data;
        char *backup_filename;
        char *intended_filename;

	static int stat = 0;
	if (stat)
		return FALSE;
	stat = 1;

        backup_filename = mate_rr_config_get_backup_filename ();
        intended_filename = mate_rr_config_get_intended_filename ();

        if (user_says_things_are_ok (confirmation->manager, confirmation->parent_window))
                unlink (backup_filename);
        else
                restore_backup_configuration (confirmation->manager, backup_filename, intended_filename, confirmation->timestamp);

        g_free (confirmation);
	stat = 0;

        return FALSE;
}

static void
queue_confirmation_by_user (UsdXrandrManager *manager, GdkWindow *parent_window, guint32 timestamp)
{
        struct confirmation *confirmation;

        confirmation = g_new (struct confirmation, 1);
        confirmation->manager = manager;
        confirmation->parent_window = parent_window;
        confirmation->timestamp = timestamp;

        g_idle_add (confirm_with_user_idle_cb, confirmation);
}

static gboolean
try_to_apply_intended_configuration (UsdXrandrManager *manager, GdkWindow *parent_window, guint32 timestamp, GError **error)
{
        char *backup_filename;
        char *intended_filename;
        gboolean result;

        /* Try to apply the intended configuration */

        backup_filename = mate_rr_config_get_backup_filename ();
        intended_filename = mate_rr_config_get_intended_filename ();

        result = apply_configuration_from_filename (manager, intended_filename, FALSE, timestamp, error);
        if (!result) {
                error_message (manager, _("The selected configuration for displays could not be applied"), error ? *error : NULL, NULL);
                restore_backup_configuration_without_messages (backup_filename, intended_filename);
                goto out;
        } else {
                /* We need to return as quickly as possible, so instead of
                 * confirming with the user right here, we do it in an idle
                 * handler.  The caller only expects a status for "could you
                 * change the RANDR configuration?", not "is the user OK with it
                 * as well?".
                 */
                queue_confirmation_by_user (manager, parent_window, timestamp);
        }

out:
        g_free (backup_filename);
        g_free (intended_filename);

        return result;
}

/* DBus method for org.ukui.SettingsDaemon.XRANDR ApplyConfiguration; see usd-xrandr-manager.xml for the interface definition */
static gboolean
usd_xrandr_manager_apply_configuration (UsdXrandrManager *manager,
                                        GError          **error)
{
        return try_to_apply_intended_configuration (manager, NULL, GDK_CURRENT_TIME, error);
}

/* DBus method for org.ukui.SettingsDaemon.XRANDR_2 ApplyConfiguration; see usd-xrandr-manager.xml for the interface definition */
static gboolean
usd_xrandr_manager_2_apply_configuration (UsdXrandrManager *manager,
                                          gint64            parent_window_id,
                                          gint64            timestamp,
                                          GError          **error)
{
        GdkWindow *parent_window;
        gboolean result;

        if (parent_window_id != 0)
                parent_window = gdk_x11_window_foreign_new_for_display (gdk_display_get_default (), (Window) parent_window_id);
        else
                parent_window = NULL;

        result = try_to_apply_intended_configuration (manager, parent_window, (guint32) timestamp, error);

        if (parent_window)
                g_object_unref (parent_window);

        return result;
}

/* We include this after the definition of usd_xrandr_manager_apply_configuration() so the prototype will already exist */
#include "usd-xrandr-manager-glue.h"

static gboolean
is_laptop (MateRRScreen *screen, MateRROutputInfo *output)
{
        MateRROutput *rr_output;

        rr_output = mate_rr_screen_get_output_by_name (screen, mate_rr_output_info_get_name (output));
        return mate_rr_output_is_laptop (rr_output);
}

static gboolean
get_clone_size (MateRRScreen *screen, int *width, int *height)
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

static void
print_output (MateRROutputInfo *info)
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

static void
print_configuration (MateRRConfig *config, const char *header)
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

static gboolean
config_is_all_off (MateRRConfig *config)
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

static MateRRConfig *
make_clone_setup (MateRRScreen *screen)
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

static MateRRMode *
find_best_mode (MateRROutput *output)
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

static gboolean
turn_on (MateRRScreen *screen,
         MateRROutputInfo *info,
         int x, int y)
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

static MateRRConfig *
make_laptop_setup (MateRRScreen *screen)
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

static int
turn_on_and_get_rightmost_offset (MateRRScreen *screen, MateRROutputInfo *info, int x)
{
        if (turn_on (screen, info, x, 0)) {
                int width;
                mate_rr_output_info_get_geometry (info, NULL, NULL, &width, NULL);
                x += width;
        }

        return x;
}

static MateRRConfig *
make_xinerama_setup (MateRRScreen *screen)
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

static MateRRConfig *
make_other_setup (MateRRScreen *screen)
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

static GPtrArray *
sanitize (UsdXrandrManager *manager, GPtrArray *array)
{
        int i;
        GPtrArray *new;

        g_debug ("before sanitizing");

        for (i = 0; i < array->len; ++i) {
                if (array->pdata[i]) {
                        print_configuration (array->pdata[i], "before");
                }
        }


        /* Remove configurations that are duplicates of
         * configurations earlier in the cycle
         */
        for (i = 0; i < array->len; i++) {
                int j;

                for (j = i + 1; j < array->len; j++) {
                        MateRRConfig *this = array->pdata[j];
                        MateRRConfig *other = array->pdata[i];

                        if (this && other && mate_rr_config_equal (this, other)) {
                                g_debug ("removing duplicate configuration");
                                g_object_unref (this);
                                array->pdata[j] = NULL;
                                break;
                        }
                }
        }

        for (i = 0; i < array->len; ++i) {
                MateRRConfig *config = array->pdata[i];

                if (config && config_is_all_off (config)) {
                        g_debug ("removing configuration as all outputs are off");
                        g_object_unref (array->pdata[i]);
                        array->pdata[i] = NULL;
                }
        }

        /* Do a final sanitization pass.  This will remove configurations that
         * don't fit in the framebuffer's Virtual size.
         */

        for (i = 0; i < array->len; i++) {
                MateRRConfig *config = array->pdata[i];

                if (config) {
                        GError *error;

                        error = NULL;
                        if (!mate_rr_config_applicable (config, manager->priv->rw_screen, &error)) { /* NULL-GError */
                                g_debug ("removing configuration which is not applicable because %s", error->message);
                                g_error_free (error);

                                g_object_unref (config);
                                array->pdata[i] = NULL;
                        }
                }
        }

        /* Remove NULL configurations */
        new = g_ptr_array_new ();

        for (i = 0; i < array->len; ++i) {
                if (array->pdata[i]) {
                        g_ptr_array_add (new, array->pdata[i]);
                        print_configuration (array->pdata[i], "Final");
                }
        }

        if (new->len > 0) {
                g_ptr_array_add (new, NULL);
        } else {
                g_ptr_array_free (new, TRUE);
                new = NULL;
        }

        g_ptr_array_free (array, TRUE);

        return new;
}

static void
generate_fn_f7_configs (UsdXrandrManager *mgr)
{
        GPtrArray *array = g_ptr_array_new ();
        MateRRScreen *screen = mgr->priv->rw_screen;

        g_debug ("Generating configurations");

        /* Free any existing list of configurations */
        if (mgr->priv->fn_f7_configs) {
                int i;

                for (i = 0; mgr->priv->fn_f7_configs[i] != NULL; ++i)
                        g_object_unref (mgr->priv->fn_f7_configs[i]);
                g_free (mgr->priv->fn_f7_configs);

                mgr->priv->fn_f7_configs = NULL;
                mgr->priv->current_fn_f7_config = -1;
        }

        g_ptr_array_add (array, mate_rr_config_new_current (screen, NULL));
        g_ptr_array_add (array, make_clone_setup (screen));
        g_ptr_array_add (array, make_xinerama_setup (screen));
        g_ptr_array_add (array, make_laptop_setup (screen));
        g_ptr_array_add (array, make_other_setup (screen));

        array = sanitize (mgr, array);

        if (array) {
                mgr->priv->fn_f7_configs = (MateRRConfig **)g_ptr_array_free (array, FALSE);
                mgr->priv->current_fn_f7_config = 0;
        }
}

static void
error_message (UsdXrandrManager *mgr, const char *primary_text, GError *error_to_display, const char *secondary_text)
{
#ifdef HAVE_LIBNOTIFY
        UsdXrandrManagerPrivate *priv = mgr->priv;
        NotifyNotification *notification;

        g_assert (error_to_display == NULL || secondary_text == NULL);

        if (priv->status_icon)
                notification = notify_notification_new (primary_text,
                                                        error_to_display ? error_to_display->message : secondary_text,
                                                        gtk_status_icon_get_icon_name(priv->status_icon));
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
#endif /* HAVE_LIBNOTIFY */
}

static void
handle_fn_f7 (UsdXrandrManager *mgr, guint32 timestamp)
{
        UsdXrandrManagerPrivate *priv = mgr->priv;
        MateRRScreen *screen = priv->rw_screen;
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
        g_debug ("Handling fn-f7");

        log_open ();
        log_msg ("Handling XF86Display hotkey - timestamp %u\n", timestamp);

        error = NULL;
        if (!mate_rr_screen_refresh (screen, &error) && error) {
                char *str;

                str = g_strdup_printf (_("Could not refresh the screen information: %s"), error->message);
                g_error_free (error);

                log_msg ("%s\n", str);
                error_message (mgr, str, NULL, _("Trying to switch the monitor configuration anyway."));
                g_free (str);
        }

        if (!priv->fn_f7_configs) {
                log_msg ("Generating stock configurations:\n");
                generate_fn_f7_configs (mgr);
                log_configurations (priv->fn_f7_configs);
        }

        current = mate_rr_config_new_current (screen, NULL);

        if (priv->fn_f7_configs &&
            (!mate_rr_config_match (current, priv->fn_f7_configs[0]) ||
             !mate_rr_config_equal (current, priv->fn_f7_configs[mgr->priv->current_fn_f7_config]))) {
                    /* Our view of the world is incorrect, so regenerate the
                     * configurations
                     */
                    generate_fn_f7_configs (mgr);
                    log_msg ("Regenerated stock configurations:\n");
                    log_configurations (priv->fn_f7_configs);
            }

        g_object_unref (current);

        if (priv->fn_f7_configs) {
                guint32 server_timestamp;
                gboolean success;

                mgr->priv->current_fn_f7_config++;

                if (priv->fn_f7_configs[mgr->priv->current_fn_f7_config] == NULL)
                        mgr->priv->current_fn_f7_config = 0;

                g_debug ("cycling to next configuration (%d)", mgr->priv->current_fn_f7_config);

                print_configuration (priv->fn_f7_configs[mgr->priv->current_fn_f7_config], "new config");

                g_debug ("applying");

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

                success = apply_configuration_and_display_error (mgr, priv->fn_f7_configs[mgr->priv->current_fn_f7_config], timestamp);

                if (success) {
                        log_msg ("Successfully switched to configuration (timestamp %u):\n", timestamp);
                        log_configuration (priv->fn_f7_configs[mgr->priv->current_fn_f7_config]);
                }
        }
        else {
                g_debug ("no configurations generated");
        }

        log_close ();

        g_debug ("done handling fn-f7");
}

static MateRROutputInfo *
get_laptop_output_info (MateRRScreen *screen, MateRRConfig *config)
{
        int i;
        MateRROutputInfo **outputs = mate_rr_config_get_outputs (config);

        for (i = 0; outputs[i] != NULL; i++) {
                if (is_laptop (screen, outputs[i]))
                        return outputs[i];
        }

        return NULL;

}

static MateRRRotation
get_next_rotation (MateRRRotation allowed_rotations, MateRRRotation current_rotation)
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

/* We use this when the XF86RotateWindows key is pressed.  That key is present
 * on some tablet PCs; they use it so that the user can rotate the tablet
 * easily.
 */
static void
handle_rotate_windows (UsdXrandrManager *mgr, guint32 timestamp)
{
        UsdXrandrManagerPrivate *priv = mgr->priv;
        MateRRScreen *screen = priv->rw_screen;
        MateRRConfig *current;
        MateRROutputInfo *rotatable_output_info;
        int num_allowed_rotations;
        MateRRRotation allowed_rotations;
        MateRRRotation next_rotation;

        g_debug ("Handling XF86RotateWindows");

        /* Which output? */

        current = mate_rr_config_new_current (screen, NULL);

        rotatable_output_info = get_laptop_output_info (screen, current);
        if (rotatable_output_info == NULL) {
                g_debug ("No laptop outputs found to rotate; XF86RotateWindows key will do nothing");
                goto out;
        }

        /* Which rotation? */

        get_allowed_rotations_for_output (current, priv->rw_screen, rotatable_output_info, &num_allowed_rotations, &allowed_rotations);
        next_rotation = get_next_rotation (allowed_rotations, mate_rr_output_info_get_rotation (rotatable_output_info));

        if (next_rotation == mate_rr_output_info_get_rotation (rotatable_output_info)) {
                g_debug ("No rotations are supported other than the current one; XF86RotateWindows key will do nothing");
                goto out;
        }

        /* Rotate */

        mate_rr_output_info_set_rotation (rotatable_output_info, next_rotation);

        apply_configuration_and_display_error (mgr, current, timestamp);

out:
        g_object_unref (current);
}

static GdkFilterReturn
event_filter (GdkXEvent           *xevent,
              GdkEvent            *event,
              gpointer             data)
{
        UsdXrandrManager *manager = data;
        XEvent *xev = (XEvent *) xevent;

        if (!manager->priv->running)
                return GDK_FILTER_CONTINUE;

        /* verify we have a key event */
        if (xev->xany.type != KeyPress && xev->xany.type != KeyRelease)
                return GDK_FILTER_CONTINUE;

        if (xev->xany.type == KeyPress) {
                if (xev->xkey.keycode == manager->priv->switch_video_mode_keycode)
                        handle_fn_f7 (manager, xev->xkey.time);
                else if (xev->xkey.keycode == manager->priv->rotate_windows_keycode)
                        handle_rotate_windows (manager, xev->xkey.time);

                return GDK_FILTER_CONTINUE;
        }

        return GDK_FILTER_CONTINUE;
}

static void
refresh_tray_icon_menu_if_active (UsdXrandrManager *manager, guint32 timestamp)
{
        UsdXrandrManagerPrivate *priv = manager->priv;

        if (priv->popup_menu) {
                gtk_menu_shell_cancel (GTK_MENU_SHELL (priv->popup_menu)); /* status_icon_popup_menu_selection_done_cb() will free everything */
                status_icon_popup_menu (manager, 0, timestamp);
        }
}



/*
 * Function: show_question()  显示缩放弹窗
 * urpose :  When the system detects the high clear screen, the pops up change scale window
 */
void show_question(GSettings *scale)
{
    GtkWidget       *dialog;
    GtkResponseType result;

    dialog = gtk_message_dialog_new(NULL,
                GTK_DIALOG_MODAL,
                GTK_MESSAGE_QUESTION,
                GTK_BUTTONS_NONE,
                _("Does the system detect high clear equipment"
                " and whether to switch to recommended scaling (200%%)?"
                " Click on the confirmation logout."));
    gtk_window_set_title(GTK_WINDOW(dialog), _("Prompt"));
    gtk_dialog_add_button(GTK_DIALOG(dialog),_("Cancel"),0);
    gtk_dialog_add_button(GTK_DIALOG(dialog),_("Confirmation"),1);

    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    if (result){
        GSettings *mouse = g_settings_new("org.ukui.peripherals-mouse");
        g_settings_set_int (mouse, "cursor-size", 36);
        g_settings_set_int (scale, XSETTINGS_KEY_SCALING, 2);
        g_object_unref (mouse);
        system("ukui-session-tools --logout");
    }
}

/*
 * Function: show_question_one() 显示缩放弹窗
 * urpose :  When the system detects the high clear screen, the pops up change scale window
 */
void show_question_one(GSettings *scale)
{
    GtkWidget       *dialog;
    GtkResponseType result;

    dialog = gtk_message_dialog_new(NULL,
                        GTK_DIALOG_MODAL,
                        GTK_MESSAGE_QUESTION,
                        GTK_BUTTONS_NONE,
                        _("The system detects that the HD device has been replaced."
                          "Do you need to switch to the recommended zoom (100%%)? "
                          "Click on the confirmation logout."));

    gtk_window_set_title(GTK_WINDOW(dialog), _("Prompt"));
    gtk_dialog_add_button(GTK_DIALOG(dialog),_("Cancel"),0);
    gtk_dialog_add_button(GTK_DIALOG(dialog),_("Confirmation"),1);

    gtk_window_set_keep_above(GTK_WINDOW(dialog), TRUE);
    gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);

    result = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    if (result){
        GSettings *mouse = g_settings_new("org.ukui.peripherals-mouse");
        g_settings_set_int (mouse, "cursor-size", 24);
        g_settings_set_int (scale, XSETTINGS_KEY_SCALING, 1);
        g_object_unref (mouse);
        system("ukui-session-tools --logout");
    }
}

/*
 * Function: one_scale_logout_dialog(int width, int height)
 * urpose :  When the system detects the high clear screen, the pops up change scale window.
 * To determine whether or not to log out and scate take effect
 */
static void
one_scale_logout_dialog(GSettings *scale)
{
    show_question_one(scale);
}

/*
 * Function: two_scale_logout_dialog(int width, int height)
 * urpose :  When the system detects the high clear screen, the pops up change scale window.
 * To determine whether or not to log out and scate take effect
 */
static void
two_scale_logout_dialog(GSettings *scale)
{
    show_question(scale);
}

static void
monitor_settings_screen_zoom(MateRRScreen *screen)
{
    MateRRConfig *config;
    MateRROutputInfo **outputs;
    int i;
    GList *just_turned_on;
    GList *l;

    gboolean OneZoom    = FALSE;
    gboolean DoubleZoom = FALSE;
    GSettings *settings = g_settings_new(XSETTINGS_SCHEMA);

    config = mate_rr_config_new_current (screen, NULL);
    just_turned_on = NULL;
    outputs = mate_rr_config_get_outputs (config);

    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];
        if (mate_rr_output_info_is_connected (output) && !mate_rr_output_info_is_active (output)) {
                just_turned_on = g_list_prepend (just_turned_on, GINT_TO_POINTER (i));
        }
    }

    for (i = 0; outputs[i] != NULL; i++) {
        MateRROutputInfo *output = outputs[i];

        if (g_list_find (just_turned_on, GINT_TO_POINTER (i)))
            continue;

        if (mate_rr_output_info_is_active (output)) {
            int width, height;
            mate_rr_output_info_get_geometry (output, NULL, NULL, &width, &height);
            /* Detect 4K screen switching and prompt whether to zoom.
             * 检测4K屏切换，并提示是否缩放
             */
            int scaling = g_settings_get_int (settings, XSETTINGS_KEY_SCALING);

            if(height > 2000 && scaling < 2){
                DoubleZoom = TRUE;//设置缩放为2倍
            }else if(height <= 2000 && scaling >= 2){
                OneZoom = TRUE;
            }
        }
    }
    for (l = just_turned_on; l; l = l->next) {
        MateRROutputInfo *output;
        int width,height;

        i = GPOINTER_TO_INT (l->data);
        output = outputs[i];

        /* since the output was off, use its preferred width/height (it doesn't have a real width/height yet) */
        width = mate_rr_output_info_get_preferred_width (output);
        height = mate_rr_output_info_get_preferred_height (output);

        /* Detect 4K screen switching and prompt whether to zoom.
         * 检测4K屏切换，并提示是否缩放
         */
        int scaling = g_settings_get_int (settings, XSETTINGS_KEY_SCALING);
        if(height > 2000 && scaling < 2 && !DoubleZoom){
            DoubleZoom = TRUE; //设置缩放为2倍
        }else if(height <= 2000 && scaling >= 2 && !OneZoom){
            OneZoom = TRUE;
        }else if (height > 2000 && scaling >= 2 && OneZoom)
            OneZoom = FALSE;
    }
    if(OneZoom)
        one_scale_logout_dialog(settings);
    else if(DoubleZoom)
        two_scale_logout_dialog(settings);

    g_object_unref (settings);
    g_list_free (just_turned_on);
    g_object_unref (config);

}

static void
auto_configure_outputs (UsdXrandrManager *manager, guint32 timestamp)
{
        UsdXrandrManagerPrivate *priv = manager->priv;
        MateRRConfig *config;
        MateRROutputInfo **outputs;
        int i;
        GList *just_turned_on;
        GList *l;
        int x;
        GError *error;
        gboolean applicable;

        config = mate_rr_config_new_current (priv->rw_screen, NULL);

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

        just_turned_on = NULL;
        outputs = mate_rr_config_get_outputs (config);

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

        x = 0;

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

        /* Second, outputs that were newly-turned on */

        for (l = just_turned_on; l; l = l->next) {
                MateRROutputInfo *output;
		        int width,height;

                i = GPOINTER_TO_INT (l->data);
                output = outputs[i];

                g_assert (mate_rr_output_info_is_active (output) && mate_rr_output_info_is_connected (output));

                /* since the output was off, use its preferred width/height (it doesn't have a real width/height yet) */
                width = mate_rr_output_info_get_preferred_width (output);
                height = mate_rr_output_info_get_preferred_height (output);
                mate_rr_output_info_set_geometry (output, x, 0, width, height);

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
                applicable = mate_rr_config_applicable (config, priv->rw_screen, &error);

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
                apply_configuration_and_display_error (manager, config, timestamp);

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

static void
apply_color_profiles (void)
{
        gboolean ret;
        GError *error = NULL;

        /* run the mate-color-manager apply program */
        ret = g_spawn_command_line_async (BINDIR "/gcm-apply", &error);
        if (!ret) {
                /* only print the warning if the binary is installed */
                if (error->code != G_SPAWN_ERROR_NOENT) {
                        g_warning ("failed to apply color profiles: %s", error->message);
                }
                g_error_free (error);
        }
}

/*查找触摸屏设备ID*/
static gboolean
find_touchscreen_device(Display* display, XIDeviceInfo *dev)
{
        int i, j;
        if (dev->use != XISlavePointer)
            return FALSE;
        if (!dev->enabled)
        {
            printf("\tThis device is disabled\n");
            return FALSE;
        }

        for (i = 0; i < dev->num_classes; i++)
        {
            if (dev->classes[i]->type == XITouchClass)
            {
                XITouchClassInfo *t = (XITouchClassInfo*)dev->classes[i];

                if (t->mode == XIDirectTouch)
                return TRUE;
            }
        }
        return FALSE;
}

/* Get device node for gudev
   node like "/dev/input/event6"
 */
unsigned char *
get_device_node (XIDeviceInfo devinfo)
{
    char *device_node;
    Atom  prop;
    int   nprops;
    char *name;

    Atom act_type;
    int  act_format;
    unsigned long nitems, bytes_after;
    unsigned char *data;


    prop = XInternAtom(GDK_DISPLAY_XDISPLAY (gdk_display_get_default()), XI_PROP_DEVICE_NODE, False);
    if (!prop)
        return NULL;

    gdk_x11_display_error_trap_push (gdk_display_get_default ());
    if (XIGetProperty(GDK_DISPLAY_XDISPLAY (gdk_display_get_default()), devinfo.deviceid, prop, 0, 1000, False,
                      AnyPropertyType, &act_type, &act_format, &nitems, &bytes_after, &data) == Success)
    {
        gdk_x11_display_error_trap_pop (gdk_display_get_default ());
        return data;
    }
    gdk_x11_display_error_trap_pop (gdk_display_get_default ());

    XFree(data);
    return NULL;
}

/* Get touchscreen info */
GList *
get_touchscreen(Display* display)
{
    gint n_devices;
    XIDeviceInfo *devs_info;
    //XIDeviceInfo *info;
    int i;
    GList *ts_devs = NULL;
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    devs_info = XIQueryDevice(dpy, XIAllDevices, &n_devices);

    for (i = 0; i < n_devices; i ++)
    {
        if (find_touchscreen_device(dpy, &devs_info[i]))
        {
           unsigned char *node;
           TsInfo *ts_info = g_new(TsInfo, 1);
           node = get_device_node (devs_info[i]);

           if (node)
           {
               ts_info->input_node = node;
               ts_info->dev_info = devs_info[i];
               ts_devs = g_list_append (ts_devs, ts_info);
           }
        }
    }
    return ts_devs;
}

/* Code fork from mutter 3.36.4 */
gboolean
check_match(int output_width, int output_height, double input_width, double input_height)
{
    double w_diff, h_diff;

    w_diff = ABS (1 - ((double) output_width / input_width));
    h_diff = ABS (1 - ((double) output_height / input_height));

    printf("w_diff is %f, h_diff is %f\n", w_diff, h_diff);

    if (w_diff < MAX_SIZE_MATCH_DIFF && h_diff < MAX_SIZE_MATCH_DIFF)
        return TRUE;
    else
        return FALSE;
}

//check if pName exit in g_TouchMapList
// IN:pName 
// OUT:pId
// RETURN: bMap
static Bool check_monitor_map(char *pName, int *pID)
{
    if((NULL == pName)||(NULL == pID))
    {
        printf("[%s %d] null\n", __FUNCTION__, __LINE__);
        return False;
    }
    Bool bMap = False;
    TouchMapInfo *pTouchMapInfo = NULL;
    GList *pIList = NULL;

    if(0 == g_list_length(g_TouchMapList))
    {
        return bMap;
    }

    for(pIList = g_TouchMapList; pIList; pIList = pIList->next)
    {
        pTouchMapInfo = pIList->data;

        printf("[%s %d] LIST[%d -- %s] Name[%s]\n", __FUNCTION__, __LINE__,
        pTouchMapInfo->touchId, pTouchMapInfo->cMonitorName, pName);

        if(strcmp(pName, pTouchMapInfo->cMonitorName))
        {
            bMap = False;
        }
        else
        {
            printf("[%s %d] bMap\n", __FUNCTION__, __LINE__);
            bMap = True;
            *pID = pTouchMapInfo->touchId;
            return bMap;
        }
    }

    return bMap;
}

// IN:id 
// OUT:pName
// RETURN: bMap
static Bool check_touch_map(int id, char *pName)
{
    if(NULL == pName)
    {
        printf("[%s %d] null\n", __FUNCTION__, __LINE__);
        return False;
    }
    Bool bMap = False;
    TouchMapInfo *pTouchMapInfo = NULL;
    GList *pIList = NULL;

    if(0 == g_list_length(g_TouchMapList))
    {
        printf("[%s %d] bMap false\n", __FUNCTION__, __LINE__);
        return bMap;
    }

    for(pIList = g_TouchMapList; pIList; pIList = pIList->next)
    {
        pTouchMapInfo = pIList->data;

        printf("[%s %d] LIST[%d -- %s] IN[%d]\n", __FUNCTION__, __LINE__,
        pTouchMapInfo->touchId, pTouchMapInfo->cMonitorName, id);

        if(id != pTouchMapInfo->touchId)
        {
            bMap = False;
        }
        else
        {
            printf("[%s %d] bMap\n", __FUNCTION__, __LINE__);
            bMap = True;
            strcpy(pName, pTouchMapInfo->cMonitorName);
            return bMap;
        }
    }
    return bMap;
}

//检测该显示器是否存在
static Bool check_monitor_exist(char *pName)
{
    Bool bExist = False;
    int i = 0;
    int iDeviceNum = 0;
    Display *pDisplay = NULL;
    XIDeviceInfo *pALLInfo = NULL;
    XRROutputInfo *pOutInfo = NULL;
    
    pDisplay = XOpenDisplay(NULL);
    if(NULL == pDisplay)
    {
        printf("[%s%d] pDisplay NULL\n", __FUNCTION__, __LINE__);
        return bExist;
    }

    pALLInfo = XIQueryDevice(pDisplay, XIAllDevices, &iDeviceNum);
    if(NULL == pALLInfo)
    {
        printf("[%s%d] pALLInfo NULL\n", __FUNCTION__, __LINE__);
        return bExist;
    }
    
    int iXScreen = DefaultScreen(pDisplay);
    Window win = RootWindow(pDisplay, iXScreen);
    XRRScreenResources *pScreenRes = XRRGetScreenResources(pDisplay, win);

    //查询所有显示器
    for(i = 0; i < pScreenRes->noutput; i++)
    {
        pOutInfo = XRRGetOutputInfo(pDisplay, pScreenRes, pScreenRes->outputs[i]);
        if(NULL == pOutInfo)
        {
            printf("[%s%d] pOutInfo NULL\n", __FUNCTION__, __LINE__);
            return bExist;
        }

        if(0 == strcmp(pName, pOutInfo->name))
        {
            printf("[%s%d] pOutInfo NULL  %s %d\n", __FUNCTION__, __LINE__,
            pOutInfo->name,pOutInfo->connection);
            bExist = True;
            break;
        }
    }

    return bExist;
}


//查找主显示器的名称 和是否映射过
static void get_primary_status(char *cName, int *pbMap)
{
    if((NULL == cName)||(NULL == pbMap))
    {
        printf("[%s%d] cName pbMap NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    int id = 0;
    Display *pDisplay = NULL;
    pDisplay = XOpenDisplay(NULL);
    if(NULL == pDisplay)
    {
        printf("[%s%d] pDisplay NULL\n", __FUNCTION__, __LINE__);
        return;
    }  

    int iMonitorNum = 0;
    int iXScreen = DefaultScreen(pDisplay);
    Window win = RootWindow(pDisplay, iXScreen);

    XRRMonitorInfo *pMonitorInfo = XRRGetMonitors(pDisplay, win, 1, &iMonitorNum);;
    if(NULL == pMonitorInfo)
    {
        printf("[%s%d] pMonitorInfo NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    strcpy(cName, XGetAtomName(pDisplay, pMonitorInfo->name));
    printf("[%s%d] name[%s] %ld \n", __FUNCTION__, __LINE__, cName, pMonitorInfo->name);

    *pbMap = check_monitor_map(cName, &id);

    return;
}

//映射中 touch id 是唯一的
static void remove_touch_map(int id)
{
    GList *pIList = NULL;
    TouchMapInfo *pTMInfo = NULL;

    for(pIList = g_TouchMapList; pIList; pIList = pIList->next)
    {
        pTMInfo = pIList->data;
        if(id == pTMInfo->touchId)
        {
            printf("[%s%d] remove[%d] %d\n", __FUNCTION__, __LINE__, pTMInfo->touchId, g_list_length(g_TouchMapList));
            g_TouchMapList = g_list_remove(g_TouchMapList, pTMInfo);
            printf("[%s%d] remove[%d] %d\n", __FUNCTION__, __LINE__, pTMInfo->touchId, g_list_length(g_TouchMapList));
        }
    }
    return;
}


typedef int (*FUNC_MAPTOOUTPUT)(Display*, char*, char*);
FUNC_MAPTOOUTPUT map_to_output = NULL;
char dllpath[64] = "/usr/lib/libkysset.so";

static int loadlib()
{
     int ret = -1;   
     void *handle  = dlopen(dllpath, RTLD_LAZY);
     if(NULL == handle)
     {
        printf("[%s%d] dlopen null\n", __FUNCTION__, __LINE__);
        return ret;
     }

     map_to_output = (FUNC_MAPTOOUTPUT)dlsym(handle, "MapToOutput");
     if(NULL == map_to_output)
     {
         printf("[%s%d] map_to_output null\n", __FUNCTION__, __LINE__); 
         return ret;
     }

     ret = Success;
     return ret;
}


/* Here to run command xinput*/
static void do_action(Display *dpy, int input_id, char *output_name)
{
    char cId[16];
    char buff[128];
    char cName[64];  //确保传参非空 此接口未用
    int ret = -1;

    if(NULL == map_to_output)
    {
        sprintf(buff, "xinput --map-to-output \"%d\" \"%s\"", input_id, output_name);
        printf("buff is %s\n", buff);
        system(buff);
    }
    else
    {
        sprintf(cId, "%d", input_id);
        ret = map_to_output(dpy, cId, output_name);
        if(Success != ret)
        {
            printf("[%s%d] map_to_output err[%d]\n", __FUNCTION__, __LINE__, ret); 
            return;
        }
        printf("[%s%d] map_to_output %s %s\n", __FUNCTION__, __LINE__, cId, output_name);  
    }

    TouchMapInfo *pTMInfo = g_new(TouchMapInfo, 1);
    strcpy(pTMInfo->cMonitorName, output_name);
    pTMInfo->touchId = input_id;

    if(!check_touch_map(input_id, cName))
    g_TouchMapList = g_list_append (g_TouchMapList, pTMInfo);

    return;
}



/* 设置触摸屏触点的角度 */
void set_touchscreen_cursor_rotation(MateRRScreen *screen)
{
    int     event_base, error_base, major, minor;
    int     o;
    Window  root;
    int     xscreen;
    XRRScreenResources *res;
    Display *dpy = XOpenDisplay(NULL);
    GList *ts_devs = NULL;

    ts_devs = get_touchscreen (dpy);

    if (!g_list_length (ts_devs))
    {
        fprintf(stdin, "No touchscreen find...\n");
        return;
    }

    GList *l = NULL;

    if (!XRRQueryExtension (dpy, &event_base, &error_base) ||
        !XRRQueryVersion (dpy, &major, &minor))
    {
        fprintf (stderr, "RandR extension missing\n");
        return;
    }

    xscreen = DefaultScreen (dpy);
    root = RootWindow (dpy, xscreen);

    if ( major >= 1 && minor >= 5)
    {
        res = XRRGetScreenResources (dpy, root);
        if (!res)
          return;

        for (o = 0; o < res->noutput; o++)
        {
            XRROutputInfo *output_info = XRRGetOutputInfo (dpy, res, res->outputs[o]);

            if (!output_info)
            {
                fprintf (stderr, "could not get output 0x%lx information\n", res->outputs[o]);
                continue;
            }

            if (output_info->connection == 0)
            {
                int output_mm_width = output_info->mm_width;
                int output_mm_height = output_info->mm_height;

                for ( l = ts_devs; l; l = l->next)
                {
                    TsInfo *info = l -> data;
                    GUdevDevice *udev_device;
                    const char *udev_subsystems[] = {"input", NULL};

                    double width, height;

                    GUdevClient *udev_client = g_udev_client_new (udev_subsystems);
                    udev_device = g_udev_client_query_by_device_file (udev_client, info->input_node);
                    if (udev_device &&
                        g_udev_device_has_property (udev_device, "ID_INPUT_WIDTH_MM"))
                    {
                        width = g_udev_device_get_property_as_double (udev_device,
                                                                    "ID_INPUT_WIDTH_MM");
                        height = g_udev_device_get_property_as_double (udev_device,
                                                                     "ID_INPUT_HEIGHT_MM");

                        char cName[64];
                        int bMap = False;
                        //id mapped
                        if(check_touch_map(info->dev_info.deviceid, cName))
                        {
                            //same map
                            if(0 == strcmp(cName, output_info->name))
                            {
                                printf("[%s%d] here\n\n", __FUNCTION__, __LINE__);
                                do_action(dpy, info->dev_info.deviceid, output_info->name);
                            }
                            //different map
                            else
                            {
                                printf("[%s%d] here %s | %s\n\n", __FUNCTION__, __LINE__, 
                                cName, output_info->name);
                                
                                //exist  : map old
                                if(check_monitor_exist(output_info->name))
                                {
                                    printf("[%s%d] here\n\n", __FUNCTION__, __LINE__);
                                    do_action(dpy, info->dev_info.deviceid, cName);
                                }
                                //unexist: remove old map, and map new
                                else
                                {
                                    printf("[%s%d] here\n\n", __FUNCTION__, __LINE__);
                                    remove_touch_map(info->dev_info.deviceid);
                                    do_action(dpy, info->dev_info.deviceid, output_info->name);
                                }
                            }
                        }
                        //id unmapped 
                        else
                        {
                            //check if primary mapped 
                            get_primary_status(cName, &bMap);
                            if(bMap)
                            {
                                printf("[%s%d] here\n\n", __FUNCTION__, __LINE__);
                                do_action(dpy, info->dev_info.deviceid, output_info->name);
                            }
                            else
                            {
                                printf("[%s%d] here\n\n", __FUNCTION__, __LINE__);
                                do_action(dpy, info->dev_info.deviceid, cName);
                            }
                        }
                    }
                    g_clear_object (&udev_client);
                }
            }
        }
    }
    else 
    {
        g_list_free(ts_devs);
        fprintf(stderr, "xrandr extension too low\n");
        return;
    }
    XCloseDisplay(dpy);
    g_list_free(ts_devs);
}

static void
on_randr_event (MateRRScreen *screen, gpointer data)
{
        UsdXrandrManager *manager = USD_XRANDR_MANAGER (data);
        UsdXrandrManagerPrivate *priv = manager->priv;
        guint32 change_timestamp, config_timestamp;
	    gboolean pop_flag = FALSE;

        if (!priv->running)
                return;

        mate_rr_screen_get_timestamps (screen, &change_timestamp, &config_timestamp);

        log_open ();
        log_msg ("Got RANDR event with timestamps change=%u %c config=%u\n",
                 change_timestamp,
                 timestamp_relationship (change_timestamp, config_timestamp),
                 config_timestamp);

        if (change_timestamp >= config_timestamp) {
                /* The event is due to an explicit configuration change.
                 *
                 * If the change was performed by us, then we need to do nothing.
                 *
                 * If the change was done by some other X client, we don't need
                 * to do anything, either; the screen is already configured.
                 */
                show_timestamps_dialog (manager, "ignoring since change > config");
                log_msg ("  Ignoring event since change >= config\n");
        } else {
                /* Here, config_timestamp > change_timestamp.  This means that
                 * the screen got reconfigured because of hotplug/unplug; the X
                 * server is just notifying us, and we need to configure the
                 * outputs in a sane way.
                 */
                char *intended_filename;
                GError *error;
                gboolean success;

                show_timestamps_dialog (manager, "need to deal with reconfiguration, as config > change");

                intended_filename = mate_rr_config_get_intended_filename ();

                error = NULL;
                success = apply_configuration_from_filename (manager, intended_filename, TRUE, config_timestamp, &error);
                g_free (intended_filename);

                if (!success) {
                        /* We don't bother checking the error type.
                         *
                         * Both G_FILE_ERROR_NOENT and
                         * MATE_RR_ERROR_NO_MATCHING_CONFIG would mean, "there
                         * was no configuration to apply, or none that matched
                         * the current outputs", and in that case we need to run
                         * our fallback.
                         *
                         * Any other error means "we couldn't do the smart thing
                         * of using a previously- saved configuration, anyway,
                         * for some other reason.  In that case, we also need to
                         * run our fallback to avoid leaving the user with a
                         * bogus configuration.
                         */

                        if (error)
                                g_error_free (error);

                        if (config_timestamp != priv->last_config_timestamp) {
                                priv->last_config_timestamp = config_timestamp;
                                auto_configure_outputs (manager, config_timestamp);
                                log_msg ("  Automatically configured outputs to deal with event\n");
                        } else
                                log_msg ("  Ignored event as old and new config timestamps are the same\n");
                } else
                        log_msg ("Applied stored configuration to deal with event\n");
                /*监听HDMI插拔设置缩放*/
                pop_flag = TRUE;
        }

        /* 添加触摸屏鼠标设置 */
        set_touchscreen_cursor_rotation(screen);

        /* poke mate-color-manager */
        apply_color_profiles ();

        refresh_tray_icon_menu_if_active (manager, MAX (change_timestamp, config_timestamp));

        /*监听HDMI插拔设置缩放*/
        if(pop_flag){
            pop_flag = FALSE;
            monitor_settings_screen_zoom(screen);
        }
        log_close ();
}

static void
run_display_capplet (GtkWidget *widget)
{
        GdkScreen *screen;
        GError *error;

        if (widget)
                screen = gtk_widget_get_screen (widget);
        else
                screen = gdk_screen_get_default ();

        error = NULL;
        if (!mate_gdk_spawn_command_line_on_screen (screen, USD_XRANDR_DISPLAY_CAPPLET, &error)) {
                GtkWidget *dialog;

                dialog = gtk_message_dialog_new_with_markup (NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                                                             "<span weight=\"bold\" size=\"larger\">"
                                                             "Display configuration could not be run"
                                                             "</span>\n\n"
                                                             "%s", error->message);
                gtk_dialog_run (GTK_DIALOG (dialog));
                gtk_widget_destroy (dialog);

                g_error_free (error);
        }
}

static void
popup_menu_configure_display_cb (GtkMenuItem *item, gpointer data)
{
        run_display_capplet (GTK_WIDGET (item));
}

static void
status_icon_popup_menu_selection_done_cb (GtkMenuShell *menu_shell, gpointer data)
{
        UsdXrandrManager *manager = USD_XRANDR_MANAGER (data);
        struct UsdXrandrManagerPrivate *priv = manager->priv;

        gtk_widget_destroy (priv->popup_menu);
        priv->popup_menu = NULL;

        mate_rr_labeler_hide (priv->labeler);
        g_object_unref (priv->labeler);
        priv->labeler = NULL;

        g_object_unref (priv->configuration);
        priv->configuration = NULL;
}

#define OUTPUT_TITLE_ITEM_BORDER 2
#define OUTPUT_TITLE_ITEM_PADDING 4

/* This is an expose-event hander for the title label for each MateRROutput.
 * We want each title to have a colored background, so we paint that background, then
 * return FALSE to let GtkLabel expose itself (i.e. paint the label's text), and then
 * we have a signal_connect_after handler as well.  See the comments below
 * to see why that "after" handler is needed.
 */
static gboolean
output_title_label_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
        UsdXrandrManager *manager = USD_XRANDR_MANAGER (data);
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        MateRROutputInfo *output;
        GdkRGBA color;
        GtkAllocation allocation;

        g_assert (GTK_IS_LABEL (widget));

        output = g_object_get_data (G_OBJECT (widget), "output");
        g_assert (output != NULL);

        g_assert (priv->labeler != NULL);

        /* Draw a black rectangular border, filled with the color that corresponds to this output */
        mate_rr_labeler_get_rgba_for_output (priv->labeler, output, &color);

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
static gboolean
output_title_label_after_draw_cb (GtkWidget *widget, cairo_t *cr, gpointer data)
{
        g_assert (GTK_IS_LABEL (widget));
        gtk_widget_set_state (widget, GTK_STATE_INSENSITIVE);

        return FALSE;
}

static void
title_item_size_allocate_cb (GtkWidget *widget, GtkAllocation *allocation, gpointer data)
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

        g_signal_handlers_block_by_func (widget, title_item_size_allocate_cb, NULL);

        /* Sigh. There is no way to turn on GTK_ALLOC_NEEDED outside of GTK+
         * itself; also, since calling size_allocate on a widget with the same
         * allcation is a no-op, we need to allocate with a "different" size
         * first.
         */

        allocation->width++;
        gtk_widget_size_allocate (widget, allocation);

        allocation->width--;
        gtk_widget_size_allocate (widget, allocation);

        g_signal_handlers_unblock_by_func (widget, title_item_size_allocate_cb, NULL);
}

static GtkWidget *
make_menu_item_for_output_title (UsdXrandrManager *manager, MateRROutputInfo *output)
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

static void
get_allowed_rotations_for_output (MateRRConfig *config,
                                  MateRRScreen *rr_screen,
                                  MateRROutputInfo *output,
                                  int *out_num_rotations,
                                  MateRRRotation *out_rotations)
{
        MateRRRotation current_rotation;
        int i;

        *out_num_rotations = 0;
        *out_rotations = 0;

        current_rotation = mate_rr_output_info_get_rotation (output);

        /* Yay for brute force */

        for (i = 0; i < G_N_ELEMENTS (possible_rotations); i++) {
                MateRRRotation rotation_to_test;

                rotation_to_test = possible_rotations[i];

                mate_rr_output_info_set_rotation (output, rotation_to_test);

                if (mate_rr_config_applicable (config, rr_screen, NULL)) { /* NULL-GError */
                        (*out_num_rotations)++;
                        (*out_rotations) |= rotation_to_test;
                }
        }

        mate_rr_output_info_set_rotation (output, current_rotation);

        if (*out_num_rotations == 0 || *out_rotations == 0) {
                g_warning ("Huh, output %p says it doesn't support any rotations, and yet it has a current rotation?", output);
                *out_num_rotations = 1;
                *out_rotations = mate_rr_output_info_get_rotation (output);
        }
}

static void
add_unsupported_rotation_item (UsdXrandrManager *manager)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *item;
        GtkWidget *label;
        gchar *markup;

        item = gtk_menu_item_new ();

        label = gtk_label_new (NULL);
        markup = g_strdup_printf ("<i>%s</i>", _("Rotation not supported"));
        gtk_label_set_markup (GTK_LABEL (label), markup);
        g_free (markup);
        gtk_container_add (GTK_CONTAINER (item), label);

        gtk_widget_show_all (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);
}

static void
ensure_current_configuration_is_saved (void)
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

static void
output_rotation_item_activate_cb (GtkCheckMenuItem *item, gpointer data)
{
        UsdXrandrManager *manager = USD_XRANDR_MANAGER (data);
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        MateRROutputInfo *output;
        MateRRRotation rotation;
        GError *error;

        /* Not interested in deselected items */
        if (!gtk_check_menu_item_get_active (item))
                return;

        ensure_current_configuration_is_saved ();

        output = g_object_get_data (G_OBJECT (item), "output");
        rotation = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), "rotation"));

        mate_rr_output_info_set_rotation (output, rotation);

        error = NULL;
        if (!mate_rr_config_save (priv->configuration, &error)) {
                error_message (manager, _("Could not save monitor configuration"), error, NULL);
                if (error)
                        g_error_free (error);

                return;
        }

        try_to_apply_intended_configuration (manager, NULL, gtk_get_current_event_time (), NULL); /* NULL-GError */
}

static void
add_items_for_rotations (UsdXrandrManager *manager, MateRROutputInfo *output, MateRRRotation allowed_rotations)
{
        typedef struct {
                MateRRRotation	rotation;
                const char *	name;
        } RotationInfo;
        static const RotationInfo rotations[] = {
                { MATE_RR_ROTATION_0, N_("Normal") },
                { MATE_RR_ROTATION_90, N_("Left") },
                { MATE_RR_ROTATION_270, N_("Right") },
                { MATE_RR_ROTATION_180, N_("Upside Down") },
                /* We don't allow REFLECT_X or REFLECT_Y for now, as mate-display-properties doesn't allow them, either */
        };

        struct UsdXrandrManagerPrivate *priv = manager->priv;
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

                item = gtk_radio_menu_item_new_with_label (group, _(rotations[i].name));
                gtk_widget_show_all (item);
                gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

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

static void
add_rotation_items_for_output (UsdXrandrManager *manager, MateRROutputInfo *output)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        int num_rotations;
        MateRRRotation rotations;

        get_allowed_rotations_for_output (priv->configuration, priv->rw_screen, output, &num_rotations, &rotations);

        if (num_rotations == 1)
                add_unsupported_rotation_item (manager);
        else
                add_items_for_rotations (manager, output, rotations);
}

static void
add_menu_items_for_output (UsdXrandrManager *manager, MateRROutputInfo *output)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *item;

        item = make_menu_item_for_output_title (manager, output);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

        add_rotation_items_for_output (manager, output);
}

static void
add_menu_items_for_outputs (UsdXrandrManager *manager)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        int i;
        MateRROutputInfo **outputs;

        outputs = mate_rr_config_get_outputs (priv->configuration);
        for (i = 0; outputs[i] != NULL; i++) {
                if (mate_rr_output_info_is_connected (outputs[i]))
                        add_menu_items_for_output (manager, outputs[i]);
        }
}

static void
status_icon_popup_menu (UsdXrandrManager *manager, guint button, guint32 timestamp)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;
        GtkWidget *item;

        g_assert (priv->configuration == NULL);
        priv->configuration = mate_rr_config_new_current (priv->rw_screen, NULL);

        g_assert (priv->labeler == NULL);
        priv->labeler = mate_rr_labeler_new (priv->configuration);

        g_assert (priv->popup_menu == NULL);
        priv->popup_menu = gtk_menu_new ();

        add_menu_items_for_outputs (manager);

        item = gtk_separator_menu_item_new ();
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

        item = gtk_menu_item_new_with_mnemonic (_("_Configure Display Settings…"));
        g_signal_connect (item, "activate",
                          G_CALLBACK (popup_menu_configure_display_cb), manager);
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (priv->popup_menu), item);

        g_signal_connect (priv->popup_menu, "selection-done",
                          G_CALLBACK (status_icon_popup_menu_selection_done_cb), manager);

        /*Set up custom theming and forced transparency support*/
        GtkWidget *toplevel = gtk_widget_get_toplevel (priv->popup_menu);
        /*Fix any failures of compiz/other wm's to communicate with gtk for transparency */
        GdkScreen *screen = gtk_widget_get_screen(GTK_WIDGET(toplevel));
        GdkVisual *visual = gdk_screen_get_rgba_visual(screen);
        gtk_widget_set_visual(GTK_WIDGET(toplevel), visual);
        /*Set up the gtk theme class from ukui-panel*/
        GtkStyleContext *context;
        context = gtk_widget_get_style_context (GTK_WIDGET(toplevel));
        gtk_style_context_add_class(context,"gnome-panel-menu-bar");
        gtk_style_context_add_class(context,"ukui-panel-menu-bar");

        gtk_menu_popup (GTK_MENU (priv->popup_menu), NULL, NULL,
                        gtk_status_icon_position_menu,
                        priv->status_icon, button, timestamp);
}

static void
status_icon_activate_cb (GtkStatusIcon *status_icon, gpointer data)
{
        UsdXrandrManager *manager = USD_XRANDR_MANAGER (data);

        /* Suck; we don't get a proper button/timestamp */
        status_icon_popup_menu (manager, 0, gtk_get_current_event_time ());
}

static void
status_icon_popup_menu_cb (GtkStatusIcon *status_icon, guint button, guint32 timestamp, gpointer data)
{
        UsdXrandrManager *manager = USD_XRANDR_MANAGER (data);

        status_icon_popup_menu (manager, button, timestamp);
}

static void
status_icon_start (UsdXrandrManager *manager)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;

        /* Ideally, we should detect if we are on a tablet and only display
         * the icon in that case.
         */
        if (!priv->status_icon) {
                priv->status_icon = gtk_status_icon_new_from_icon_name (USD_XRANDR_ICON_NAME);
                gtk_status_icon_set_tooltip_text (priv->status_icon, _("Configure display settings"));

                g_signal_connect (priv->status_icon, "activate",
                                  G_CALLBACK (status_icon_activate_cb), manager);
                g_signal_connect (priv->status_icon, "popup-menu",
                                  G_CALLBACK (status_icon_popup_menu_cb), manager);
        }
}

static void
status_icon_stop (UsdXrandrManager *manager)
{
        struct UsdXrandrManagerPrivate *priv = manager->priv;

        if (priv->status_icon) {
                g_signal_handlers_disconnect_by_func (
                        priv->status_icon, G_CALLBACK (status_icon_activate_cb), manager);
                g_signal_handlers_disconnect_by_func (
                        priv->status_icon, G_CALLBACK (status_icon_popup_menu_cb), manager);

                /* hide the icon before unreffing it; otherwise we will leak
                   whitespace in the notification area due to a bug in there */
                gtk_status_icon_set_visible (priv->status_icon, FALSE);
                g_object_unref (priv->status_icon);
                priv->status_icon = NULL;
        }
}

static void
start_or_stop_icon (UsdXrandrManager *manager)
{
        if (g_settings_get_boolean (manager->priv->settings, CONF_KEY_SHOW_NOTIFICATION_ICON)) {
                status_icon_start (manager);
        }
        else {
                status_icon_stop (manager);
        }
}

static void
on_config_changed (GSettings        *settings,
                   gchar            *key,
                   UsdXrandrManager *manager)
{
        if (g_strcmp0 (key, CONF_KEY_SHOW_NOTIFICATION_ICON) == 0)
                start_or_stop_icon (manager);
}

static gboolean
apply_intended_configuration (UsdXrandrManager *manager, const char *intended_filename, guint32 timestamp)
{
        GError *my_error;
        gboolean result;

        my_error = NULL;
        result = apply_configuration_from_filename (manager, intended_filename, TRUE, timestamp, &my_error);
        if (!result) {
                if (my_error) {
                        if (!g_error_matches (my_error, G_FILE_ERROR, G_FILE_ERROR_NOENT) &&
                            !g_error_matches (my_error, MATE_RR_ERROR, MATE_RR_ERROR_NO_MATCHING_CONFIG))
                                error_message (manager, _("Could not apply the stored configuration for monitors"), my_error, NULL);

                        g_error_free (my_error);
                }
        }

        return result;
}

static void
apply_default_boot_configuration (UsdXrandrManager *mgr, guint32 timestamp)
{
        UsdXrandrManagerPrivate *priv = mgr->priv;
        MateRRScreen *screen = priv->rw_screen;
        MateRRConfig *config;
        gboolean turn_on_external, turn_on_laptop;

        turn_on_external =
                g_settings_get_boolean (mgr->priv->settings, CONF_KEY_TURN_ON_EXTERNAL_MONITORS_AT_STARTUP);
        turn_on_laptop =
                g_settings_get_boolean (mgr->priv->settings, CONF_KEY_TURN_ON_LAPTOP_MONITOR_AT_STARTUP);

        if (turn_on_external && turn_on_laptop)
                config = make_clone_setup (screen);
        else if (!turn_on_external && turn_on_laptop)
                config = make_laptop_setup (screen);
        else if (turn_on_external && !turn_on_laptop)
                config = make_other_setup (screen);
        else
                config = make_laptop_setup (screen);

        if (config) {
                apply_configuration_and_display_error (mgr, config, timestamp);
                g_object_unref (config);
        }
}

static gboolean
apply_stored_configuration_at_startup (UsdXrandrManager *manager, guint32 timestamp)
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

static gboolean
apply_default_configuration_from_file (UsdXrandrManager *manager, guint32 timestamp)
{
        UsdXrandrManagerPrivate *priv = manager->priv;
        char *default_config_filename;
        gboolean result;

        default_config_filename = g_settings_get_string (priv->settings, CONF_KEY_DEFAULT_CONFIGURATION_FILE);
        if (!default_config_filename)
                return FALSE;

        result = apply_configuration_from_filename (manager, default_config_filename, TRUE, timestamp, NULL);

        g_free (default_config_filename);
        return result;
}


//IN: 触摸屏的ID
void set_touch_map(int touchId)
{
    char cName[64];
    int id = 0;
    //该ID映射过则不再映射
    if(check_touch_map(touchId, cName))
    {
        printf("[%s%d] touchId[%d] has been mapped\n", __FUNCTION__, __LINE__, touchId);
        return;
    }

    int i = 0;
    Bool bMapOk = False;  //触摸屏映射成功标志
    Bool bPrimaryMapped = False; //主屏映射过标志
    char cPrimaryName[64];
    Display *pDisplay = NULL;
    XRROutputInfo *pOutInfo = NULL;

    get_primary_status(cPrimaryName, &bPrimaryMapped);
    printf("[%s%d] name[%s] %d \n", __FUNCTION__, __LINE__, cPrimaryName, bPrimaryMapped);

    pDisplay = XOpenDisplay(NULL);
    if(NULL == pDisplay)
    {
        printf("[%s%d] pDisplay NULL\n", __FUNCTION__, __LINE__);
        return;
    }

    int iXScreen = DefaultScreen(pDisplay);
    Window win = RootWindow(pDisplay, iXScreen);
    XRRScreenResources *pScreenRes = XRRGetScreenResources(pDisplay, win);

    //主屏未映射，则映射在主屏
    //主屏已映射，则继续查找映射在未映射过的屏上
    if(False == bPrimaryMapped)
    {
        //查询所有显示器
        for(i = 0; i < pScreenRes->noutput; i++)
        {
            pOutInfo = XRRGetOutputInfo(pDisplay, pScreenRes, pScreenRes->outputs[i]);
            if(NULL == pOutInfo)
            {
                printf("[%s%d] pOutInfo NULL\n", __FUNCTION__, __LINE__);
                return;
            }

            if(0 == strcmp(cPrimaryName, pOutInfo->name))
            {
                printf("[%s%d] ---- \n", __FUNCTION__, __LINE__);
                do_action(pDisplay, touchId, pOutInfo->name);
                bMapOk = True;
                break;
            }  
        }
    }
    else
    {
        //查询所有显示器
        for(i = 0; i < pScreenRes->noutput; i++)
        {
            pOutInfo = XRRGetOutputInfo(pDisplay, pScreenRes, pScreenRes->outputs[i]);
            if(NULL == pOutInfo)
            {
                printf("[%s%d] pOutInfo NULL\n", __FUNCTION__, __LINE__);
                return;
            }

            if(!check_monitor_map(pOutInfo->name, &id))
            {
                printf("[%s%d] ---- \n", __FUNCTION__, __LINE__);
                do_action(pDisplay, touchId, pOutInfo->name);
                bMapOk = True;
                break;
            }  
        }
    }
    
    //查找所有显示器都未成功映射，则映射到主显示器上
    if(False == bMapOk)
    {
        do_action(pDisplay, touchId, cPrimaryName);
        bMapOk = True;
    }

    XCloseDisplay(pDisplay);

    return;    
}
void listen_to_Xinput_Event()
{
    Display *pDisplay = NULL;
    pDisplay = XOpenDisplay(NULL);
    XEvent stEvent;
    XGenericEventCookie *pCookie = NULL;
    if(NULL == pDisplay)
    {
        printf("xopendisplay failed\n");
        return;
    }
    int nInputDev = 0;
    XDeviceInfo *pAllXDevInfo = NULL;
    XDeviceInfo *pPreXDevInfo = NULL;

    XIEventMask mask[2];
    XIEventMask *m;
    Window win;
    win = DefaultRootWindow(pDisplay);
    m = &mask[0];
    m->deviceid = XIAllDevices;
    m->mask_len = XIMaskLen(XI_LASTEVENT);
    m->mask = (unsigned char*)calloc(m->mask_len, sizeof(char));
    XISetMask(m->mask, XI_HierarchyChanged);

    m = &mask[1];
    m->deviceid = XIAllMasterDevices;
    m->mask_len = XIMaskLen(XI_LASTEVENT);
    m->mask = (unsigned char*)calloc(m->mask_len, sizeof(char));

    XISelectEvents(pDisplay, win, &mask[0], 2);
    XSync(pDisplay, False);

    free(mask[0].mask);
    free(mask[1].mask);

    while(1)
    {
        pCookie = (XGenericEventCookie*)&stEvent.xcookie;
        XNextEvent(pDisplay, &stEvent);

        if(XGetEventData(pDisplay, pCookie) && (GenericEvent == pCookie->type))
        {
            if(XI_HierarchyChanged == pCookie->evtype)
            {
                XIHierarchyEvent *pHev = (XIHierarchyEvent*)pCookie->data;
                pAllXDevInfo = XListInputDevices(pDisplay, &nInputDev);
                if(NULL == pAllXDevInfo)
                {
                    printf("[%s%d]pAllXDevInfo null\n", __FUNCTION__, __LINE__);
                    return;
                }

                pPreXDevInfo = &pAllXDevInfo[nInputDev-1];
                if(NULL == pPreXDevInfo)
                {
                    printf("[%s%d]pPreXDevInfo null\n", __FUNCTION__, __LINE__);
                    return;
                }

                
                switch (pHev->flags)
                {
                    case XISlaveAdded:
                            printf("[%s%d] id=%ld \n",__FUNCTION__, __LINE__, pPreXDevInfo->id);
                            
                            if(XInternAtom(pDisplay, XI_TOUCHSCREEN, True) == pPreXDevInfo->type)
                            set_touch_map(pPreXDevInfo->id);
                            break;
                    case XISlaveRemoved:
                            
                            remove_touch_map(pHev->info[pHev->num_info-1].deviceid);
                            break;
                    default:
                            //printf("flag is %d \n", pHev->flags);
                            break;
                }
            }
        }

        XFreeEventData(pDisplay, pCookie);
        usleep(50*1000);
    }
    XCloseDisplay(pDisplay);
    return ;
}

gboolean
usd_xrandr_manager_start (UsdXrandrManager *manager,
                          GError          **error)
{
        GdkDisplay      *display;
        g_debug ("Starting xrandr manager");
        ukui_settings_profile_start (NULL);

        log_open ();
        log_msg ("------------------------------------------------------------\nSTARTING XRANDR PLUGIN\n");

        loadlib();

        manager->priv->rw_screen = mate_rr_screen_new (gdk_screen_get_default (), error);

        if (manager->priv->rw_screen == NULL) {
                log_msg ("Could not initialize the RANDR plugin%s%s\n",
                         (error && *error) ? ": " : "",
                         (error && *error) ? (*error)->message : "");
                log_close ();
                return FALSE;
        }

        g_signal_connect (manager->priv->rw_screen, "changed", G_CALLBACK (on_randr_event), manager);

        log_msg ("State of screen at startup:\n");
        log_screen (manager->priv->rw_screen);

        manager->priv->running = TRUE;
        manager->priv->settings = g_settings_new (CONF_SCHEMA);

        g_signal_connect (manager->priv->settings,
                          "changed::" CONF_KEY_SHOW_NOTIFICATION_ICON,
                          G_CALLBACK (on_config_changed),
                          manager);

        display = gdk_display_get_default ();
        if (manager->priv->switch_video_mode_keycode) {
                gdk_x11_display_error_trap_push (display);

                XGrabKey (gdk_x11_get_default_xdisplay(),
                          manager->priv->switch_video_mode_keycode, AnyModifier,
                          gdk_x11_get_default_root_xwindow(),
                          True, GrabModeAsync, GrabModeAsync);

                gdk_display_flush (display);
                gdk_x11_display_error_trap_pop_ignored (display);
        }

        if (manager->priv->rotate_windows_keycode) {
                gdk_x11_display_error_trap_push (display);

                XGrabKey (gdk_x11_get_default_xdisplay(),
                          manager->priv->rotate_windows_keycode, AnyModifier,
                          gdk_x11_get_default_root_xwindow(),
                          True, GrabModeAsync, GrabModeAsync);

                gdk_display_flush (display);
                gdk_x11_display_error_trap_pop_ignored (display);
        }

        show_timestamps_dialog (manager, "Startup");
        if (!apply_stored_configuration_at_startup (manager, GDK_CURRENT_TIME)) /* we don't have a real timestamp at startup anyway */
                if (!apply_default_configuration_from_file (manager, GDK_CURRENT_TIME))
                        if (!g_settings_get_boolean (manager->priv->settings, CONF_KEY_USE_XORG_MONITOR_SETTINGS))
                                apply_default_boot_configuration (manager, GDK_CURRENT_TIME);

        log_msg ("State of screen after initial configuration:\n");
        log_screen (manager->priv->rw_screen);

        gdk_window_add_filter (gdk_get_default_root_window(),
                               (GdkFilterFunc)event_filter,
                               manager);

        /* 添加触摸屏鼠标设置 */
        set_touchscreen_cursor_rotation(manager->priv->rw_screen);


        start_or_stop_icon (manager);

        log_close ();

        ukui_settings_profile_end (NULL);

        int pthr_listenXinput = -1;
        pthread_t thread;
        pthr_listenXinput = pthread_create(&thread, NULL, (void *)&listen_to_Xinput_Event, NULL);
        if(0 != pthr_listenXinput)
        {
            printf("[%s%d] creat thread failed\n", __FUNCTION__, __LINE__);
        }

        return TRUE;
}

void
usd_xrandr_manager_stop (UsdXrandrManager *manager)
{
        GdkDisplay      *display;
        g_debug ("Stopping xrandr manager");

        manager->priv->running = FALSE;

        display = gdk_display_get_default ();

        if (manager->priv->switch_video_mode_keycode) {
                gdk_x11_display_error_trap_push (display);

                XUngrabKey (gdk_x11_get_default_xdisplay(),
                            manager->priv->switch_video_mode_keycode, AnyModifier,
                            gdk_x11_get_default_root_xwindow());

                gdk_x11_display_error_trap_pop_ignored (display);
        }

        if (manager->priv->rotate_windows_keycode) {
                gdk_x11_display_error_trap_push (display);

                XUngrabKey (gdk_x11_get_default_xdisplay(),
                            manager->priv->rotate_windows_keycode, AnyModifier,
                            gdk_x11_get_default_root_xwindow());

                gdk_x11_display_error_trap_pop_ignored (display);
        }

        gdk_window_remove_filter (gdk_get_default_root_window (),
                                  (GdkFilterFunc) event_filter,
                                  manager);

        if (manager->priv->settings != NULL) {
                g_object_unref (manager->priv->settings);
                manager->priv->settings = NULL;
        }

        if (manager->priv->rw_screen != NULL) {
                g_object_unref (manager->priv->rw_screen);
                manager->priv->rw_screen = NULL;
        }

        if (manager->priv->dbus_connection != NULL) {
                dbus_g_connection_unref (manager->priv->dbus_connection);
                manager->priv->dbus_connection = NULL;
        }

        status_icon_stop (manager);

        log_open ();
        log_msg ("STOPPING XRANDR PLUGIN\n------------------------------------------------------------\n");
        log_close ();
}

static void
usd_xrandr_manager_class_init (UsdXrandrManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = usd_xrandr_manager_finalize;

        dbus_g_object_type_install_info (USD_TYPE_XRANDR_MANAGER, &dbus_glib_usd_xrandr_manager_object_info);

        g_type_class_add_private (klass, sizeof (UsdXrandrManagerPrivate));
}

static guint
get_keycode_for_keysym_name (const char *name)
{
        Display *dpy;
        guint keyval;

        dpy = gdk_x11_get_default_xdisplay ();
        keyval = gdk_keyval_from_name (name);
        return XKeysymToKeycode (dpy, keyval);
}

static void
usd_xrandr_manager_init (UsdXrandrManager *manager)
{
        manager->priv = USD_XRANDR_MANAGER_GET_PRIVATE (manager);

        manager->priv->switch_video_mode_keycode = get_keycode_for_keysym_name (VIDEO_KEYSYM);
        manager->priv->rotate_windows_keycode = get_keycode_for_keysym_name (ROTATE_KEYSYM);

        manager->priv->current_fn_f7_config = -1;
        manager->priv->fn_f7_configs = NULL;
}

static void
usd_xrandr_manager_finalize (GObject *object)
{
        UsdXrandrManager *xrandr_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (USD_IS_XRANDR_MANAGER (object));

        xrandr_manager = USD_XRANDR_MANAGER (object);

        g_return_if_fail (xrandr_manager->priv != NULL);

        G_OBJECT_CLASS (usd_xrandr_manager_parent_class)->finalize (object);
}

static gboolean
register_manager_dbus (UsdXrandrManager *manager)
{
        GError *error = NULL;

        manager->priv->dbus_connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (manager->priv->dbus_connection == NULL) {
                if (error != NULL) {
                        g_warning ("Error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        /* Hmm, should we do this in usd_xrandr_manager_start()? */
        dbus_g_connection_register_g_object (manager->priv->dbus_connection, USD_XRANDR_DBUS_PATH, G_OBJECT (manager));

        return TRUE;
}

UsdXrandrManager *
usd_xrandr_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (USD_TYPE_XRANDR_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);

                if (!register_manager_dbus (manager_object)) {
                        g_object_unref (manager_object);
                        return NULL;
                }
        }

        return USD_XRANDR_MANAGER (manager_object);
}
