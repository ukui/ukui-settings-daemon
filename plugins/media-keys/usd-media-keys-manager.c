/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2001-2003 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2006-2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2014 Michal Ratajsky <michal.ratajsky@gmail.com>
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <pthread.h>
#include <X11/Xlib.h>
#include <X11/Xlibint.h>
#include <X11/XKBlib.h>
#include <X11/extensions/record.h>
#include <X11/keysym.h>

#ifdef HAVE_LIBMATEMIXER
#include <libmatemixer/matemixer.h>
#endif

#ifdef HAVE_LIBCANBERRA
#include <canberra-gtk.h>
#endif

#include "ukui-settings-profile.h"
#include "usd-marshal.h"
#include "usd-media-keys-manager.h"
#include "usd-media-keys-manager-glue.h"

#include "eggaccelerators.h"
#include "acme.h"
#include "usd-media-keys-window.h"
#include "usd-input-helper.h"

#define USD_DBUS_PATH "/org/ukui/SettingsDaemon"
#define USD_DBUS_NAME "org.ukui.SettingsDaemon"
#define USD_MEDIA_KEYS_DBUS_PATH USD_DBUS_PATH "/MediaKeys"
#define USD_MEDIA_KEYS_DBUS_NAME USD_DBUS_NAME ".MediaKeys"

#define TOUCHPAD_SCHEMA "org.ukui.peripherals-touchpad"
#define TOUCHPAD_ENABLED_KEY "touchpad-enabled"

#define POINTER_SCHEMA  "org.ukui.SettingsDaemon.plugins.mouse"
#define POINTER_KEY     "locate-pointer"

#define SESSION_SCHEMA  "org.ukui.session"
#define WIN_KEY         "win-key-release"

#define SCREENSHOT_SCHEMA   "org.ukui.screenshot"
#define RUNINGS_KEY         "isrunning"

#define VOLUME_STEP 6

#define USD_MEDIA_KEYS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), USD_TYPE_MEDIA_KEYS_MANAGER, UsdMediaKeysManagerPrivate))


typedef struct {
        char   *application;
        guint32 time;
} MediaPlayer;

struct _UsdMediaKeysManagerPrivate
{
#ifdef HAVE_LIBMATEMIXER
        /* Volume bits */
        MateMixerContext       *context;
        MateMixerStream        *stream;
        MateMixerStream        *input_stream;
        MateMixerStreamControl *input_control;
        MateMixerStreamControl *control;
#endif
        GtkWidget        *volume_dialog;
        GtkWidget        *dialog;
        GSettings        *settings;
        GSettings        *point_settings;
        GSettings        *session_settings;
        GSettings        *screenshot_settings;

        GVolumeMonitor   *volume_monitor;

        /* Multihead stuff */
        GdkScreen        *current_screen;
        GSList           *screens;

        GList            *media_players;

        DBusGConnection  *connection;
        guint             notify[HANDLED_KEYS];
        GDBusProxy *bproxy;
        GDBusConnection *bcon;

        GHashTable       *hash;
};

enum {
        MEDIA_PLAYER_KEY_PRESSED,
        LAST_SIGNAL
};

static const int *ModifiersVec[] = {
    XK_Control_L,
    XK_Control_R,
    XK_Shift_L,
    XK_Shift_R,
    XK_Super_L,
    XK_Super_R,
    XK_Alt_L,
    XK_Alt_R,
    NULL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void     usd_media_keys_manager_class_init  (UsdMediaKeysManagerClass *klass);
static void     usd_media_keys_manager_init        (UsdMediaKeysManager      *media_keys_manager);

G_DEFINE_TYPE (UsdMediaKeysManager, usd_media_keys_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static void
init_screens (UsdMediaKeysManager *manager)
{
        GdkDisplay *display;
        int i;

        display = gdk_display_get_default ();
        for (i = 0; i < gdk_display_get_n_screens (display); i++) {
                GdkScreen *screen;

                screen = gdk_display_get_screen (display, i);
                if (screen == NULL) {
                        continue;
                }
                manager->priv->screens = g_slist_append (manager->priv->screens, screen);
        }

        manager->priv->current_screen = manager->priv->screens->data;
}

static void
acme_error (char * msg)
{
        GtkWidget *error_dialog;

        error_dialog = gtk_message_dialog_new (NULL,
                                               GTK_DIALOG_MODAL,
                                               GTK_MESSAGE_ERROR,
                                               GTK_BUTTONS_OK,
                                               msg, NULL);
        gtk_dialog_set_default_response (GTK_DIALOG (error_dialog),
                                         GTK_RESPONSE_OK);
        gtk_widget_show (error_dialog);
        g_signal_connect (error_dialog,
                          "response",
                          G_CALLBACK (gtk_widget_destroy),
                          NULL);
}

static char *
get_term_command (UsdMediaKeysManager *manager)
{
	char *cmd_term, *cmd_args;
	char *cmd = NULL;
	GSettings *settings;

	settings = g_settings_new ("org.mate.applications-terminal");
	cmd_term = g_settings_get_string (settings, "exec");
	cmd_args = g_settings_get_string (settings, "exec-arg");

	if (cmd_term[0] != '\0') {
		cmd = g_strdup_printf ("%s %s -e", cmd_term, cmd_args);
	} else {
		cmd = g_strdup_printf ("ukui-terminal -e");
	}

	g_free (cmd_args);
	g_free (cmd_term);
	g_object_unref (settings);

        return cmd;
}

static void
execute (UsdMediaKeysManager *manager,
         char                *cmd,
         gboolean             sync,
         gboolean             need_term)
{
        gboolean retval;
        char   **argv;
        int      argc;
        char    *exec;
        char    *term = NULL;

        retval = FALSE;

        if (need_term) {
                term = get_term_command (manager);
                if (term == NULL) {
                        acme_error (_("Could not get default terminal. Verify that your default "
                                      "terminal command is set and points to a valid application."));
                        return;
                }
        }

        if (term) {
                exec = g_strdup_printf ("%s %s", term, cmd);
                g_free (term);
        } else {
                exec = g_strdup (cmd);
        }

        if (g_shell_parse_argv (exec, &argc, &argv, NULL)) {
                if (sync != FALSE) {
                        retval = g_spawn_sync (g_get_home_dir (),
                                               argv,
                                               NULL,
                                               G_SPAWN_SEARCH_PATH,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL,
                                               NULL);
                } else {
                        retval = g_spawn_async (g_get_home_dir (),
                                                argv,
                                                NULL,
                                                G_SPAWN_SEARCH_PATH,
                                                NULL,
                                                NULL,
                                                NULL,
                                                NULL);
                }
                g_strfreev (argv);
        }

        if (retval == FALSE) {
                char *msg;
                msg = g_strdup_printf (_("Couldn't execute command: %s\n"
                                         "Verify that this is a valid command."),
                                       exec);

                acme_error (msg);
                g_free (msg);
        }
        g_free (exec);
}

static void
dialog_init (UsdMediaKeysManager *manager)
{
        if (manager->priv->dialog != NULL
            && !usd_osd_window_is_valid (USD_OSD_WINDOW (manager->priv->dialog))) {
                gtk_widget_destroy (manager->priv->dialog);
                manager->priv->dialog = NULL;
        }

        if (manager->priv->dialog == NULL) {
                manager->priv->dialog = usd_media_keys_window_new ();
        }
}
static void
volume_dialog_init (UsdMediaKeysManager *manager)
{
        if (manager->priv->volume_dialog != NULL
            && !usd_osd_window_is_valid (USD_OSD_WINDOW (manager->priv->volume_dialog))) {
                gtk_widget_destroy (manager->priv->volume_dialog);
                manager->priv->volume_dialog = NULL;
        }

        if (manager->priv->volume_dialog == NULL) {
                manager->priv->volume_dialog = usd_media_keys_window_new ();
        }
}

static gboolean
is_valid_shortcut (const char *string)
{
        if (string == NULL || string[0] == '\0') {
                return FALSE;
        }
        if (strcmp (string, "disabled") == 0) {
                return FALSE;
        }

        return TRUE;
}

static void
update_kbd_cb (GSettings           *settings,
               gchar               *settings_key,
               UsdMediaKeysManager *manager)
{
        int      i;
        gboolean need_flush = TRUE;

        g_return_if_fail (settings_key != NULL);

        gdk_x11_display_error_trap_push (gdk_display_get_default ());
        /* Find the key that was modified */
        for (i = 0; i < HANDLED_KEYS; i++) {
                if (g_strcmp0 (settings_key, keys[i].settings_key) == 0) {
                        char *tmp;
                        Key  *key;

                        if (keys[i].key != NULL) {
                                need_flush = TRUE;
                                grab_key_unsafe (keys[i].key, FALSE, manager->priv->screens);
                        }

                        g_free (keys[i].key);
                        keys[i].key = NULL;

                        /* We can't have a change in a hard-coded key */
                        g_assert (keys[i].settings_key != NULL);

                        tmp = g_settings_get_string (settings,
                                                     keys[i].settings_key);

                        if (is_valid_shortcut (tmp) == FALSE) {
                                g_free (tmp);
                                break;
                        }

                        key = g_new0 (Key, 1);
                        if (!egg_accelerator_parse_virtual (tmp, &key->keysym, &key->keycodes, &key->state)) {
                                g_free (tmp);
                                g_free (key);
                                break;
                        }

                        need_flush = TRUE;
                        grab_key_unsafe (key, TRUE, manager->priv->screens);
                        keys[i].key = key;

                        g_free (tmp);

                        break;
                }
        }

        if (need_flush)
                gdk_flush ();
        if (gdk_x11_display_error_trap_pop (gdk_display_get_default ()))
                g_warning ("Grab failed for some keys, another application may already have access the them.");
}

static void init_kbd(UsdMediaKeysManager* manager)
{
	int i;
	gboolean need_flush = FALSE;

	ukui_settings_profile_start(NULL);

	gdk_x11_display_error_trap_push (gdk_display_get_default ());
	for (i = 0; i < HANDLED_KEYS; i++)
	{
		char* tmp;
		Key* key;

		gchar* signal_name;

        if(g_strcmp0 (keys[i].settings_key, "screenshot") == 0)
              continue;

		signal_name = g_strdup_printf ("changed::%s", keys[i].settings_key);
		g_signal_connect (manager->priv->settings,
						  signal_name,
						  G_CALLBACK (update_kbd_cb),
						  manager);
		g_free (signal_name);

		if (keys[i].settings_key != NULL) {
			tmp = g_settings_get_string (manager->priv->settings, keys[i].settings_key);
		} else {
			tmp = g_strdup (keys[i].hard_coded);
		}

		if (!is_valid_shortcut(tmp))
		{
			g_debug("Not a valid shortcut: '%s'", tmp);
			g_free(tmp);
			continue;
		}

		key = g_new0(Key, 1);

		if (!egg_accelerator_parse_virtual(tmp, &key->keysym, &key->keycodes, &key->state))
		{
			g_debug("Unable to parse: '%s'", tmp);
			g_free(tmp);
			g_free(key);
			continue;
		}

		g_free(tmp);

		keys[i].key = key;

		need_flush = TRUE;
		grab_key_unsafe(key, TRUE, manager->priv->screens);
	}

	if (need_flush)
	{
		gdk_flush();
	}

	if (gdk_x11_display_error_trap_pop (gdk_display_get_default ()))
	{
		g_warning("Grab failed for some keys, another application may already have access the them.");
	}

	ukui_settings_profile_end(NULL);
}

int gDbus_proxy_call_for_panel_posotion(UsdMediaKeysManager *manager)
{
        GVariant *result;
        int ukui_panel_position;

        GError *error = NULL;
        result =  g_dbus_proxy_call_sync (manager->priv->bproxy, "GetPanelPosition", g_variant_new ("(s)", ""), G_DBUS_CALL_FLAGS_NONE, -1, NULL,  &error);
        g_variant_get (result, "(i)", &ukui_panel_position);

        g_print ("ukui_panel_position = : %d \n", ukui_panel_position);

        return ukui_panel_position;
}

static void
dialog_show (UsdMediaKeysManager *manager)
{
        int            orig_w;
        int            orig_h;
        int            screen_w;
        int            screen_h;
        int            x;
        int            y;
        GdkDisplay *display;
        GdkDeviceManager *device_manager;
        GdkSeat       *seat;
        GdkDevice *pointer;
        int            pointer_x;
        int            pointer_y;
        GtkRequisition win_req;
        GdkScreen     *pointer_screen;
        GdkRectangle   geometry;
        int            monitor;
        int ukui_panel_position;

        /*get ukui_panel position*/
        ukui_panel_position = gDbus_proxy_call_for_panel_posotion(manager);

        gtk_window_set_screen (GTK_WINDOW (manager->priv->dialog),
                               manager->priv->current_screen);

        /* Return if OSD notifications are disabled */
        if (!g_settings_get_boolean (manager->priv->settings, "enable-osd"))
                return;

        /*
         * get the window size
         * if the window hasn't been mapped, it doesn't necessarily
         * know its true size, yet, so we need to jump through hoops
         */
        gtk_window_get_default_size (GTK_WINDOW (manager->priv->dialog), &orig_w, &orig_h);
        gtk_widget_get_preferred_size (manager->priv->dialog, NULL, &win_req);

        if (win_req.width > orig_w) {
                orig_w = win_req.width;
        }
        if (win_req.height > orig_h) {
                orig_h = win_req.height;
        }

        pointer_screen = NULL;
        display = gdk_screen_get_display (manager->priv->current_screen);
        seat = gdk_display_get_default_seat (display);
        pointer = gdk_seat_get_pointer (seat);

        gdk_device_get_position (pointer,
                                 &pointer_screen,
                                 &pointer_x,
                                 &pointer_y);

        if (pointer_screen != manager->priv->current_screen) {
                /* The pointer isn't on the current screen, so just
                 * assume the default monitor
                 */
                monitor = 0;// gdk_display_get_monitor (display, 0);
        } else {
                monitor = gdk_display_get_monitor_at_point (manager->priv->current_screen,
                                                            pointer_x, 
                                                            pointer_y);
        }
        gdk_screen_get_monitor_geometry (manager->priv->current_screen,
                                         monitor,
                                         &geometry);
        //gdk_monitor_get_geometry (monitor, &geometry);
        orig_w = 144;
        orig_h = 144;
        screen_w = geometry.width;
        screen_h = geometry.height;
        x = screen_w - orig_w - 200;//((screen_w - orig_w) / 2) + geometry.x;
        y = screen_h - orig_h - 100;//geometry.y + (screen_h / 2) + (screen_h / 2 - orig_h) / 2;

        g_print(" screen_w = %d, screen_h = %d, x = %d, y = %d", screen_w, screen_h, x, y);
        gtk_window_set_default_size(GTK_WINDOW (manager->priv->dialog), orig_w ,orig_h);
        gtk_window_move (GTK_WINDOW (manager->priv->dialog), x, y);
        
        GtkStyleContext * context = gtk_widget_get_style_context (manager->priv->dialog);
        gtk_style_context_save (context);
        GtkCssProvider *provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_data(provider, ".volume-box { border-radius:6px; background:rgba(19,20,20,0.6);}", -1, NULL);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
        gtk_style_context_add_class (context, "volume-box");
        g_object_unref (provider);

        gtk_widget_show (manager->priv->dialog);

        gdk_display_sync (gdk_screen_get_display (manager->priv->current_screen));

}
static void
volume_dialog_show (UsdMediaKeysManager *manager)
{
        int            orig_w;
        int            orig_h;
        int            screen_w;
        int            screen_h;
        int            x;
        int            y;
        GdkDisplay *display;
        GdkDeviceManager *device_manager;
        GdkSeat       *seat;
        GdkDevice *pointer;
        int            pointer_x;
        int            pointer_y;
        GtkRequisition win_req;
        GdkScreen     *pointer_screen;
        GdkRectangle   geometry;
        int            monitor;
        int ukui_panel_position;

        /*get ukui_panel position*/
        ukui_panel_position = gDbus_proxy_call_for_panel_posotion(manager);

        gtk_window_set_screen (GTK_WINDOW (manager->priv->volume_dialog),
                               manager->priv->current_screen);

        /* Return if OSD notifications are disabled */
        if (!g_settings_get_boolean (manager->priv->settings, "enable-osd"))
                return;

        /*
         * get the window size
         * if the window hasn't been mapped, it doesn't necessarily
         * know its true size, yet, so we need to jump through hoops
         */
        gtk_window_get_default_size (GTK_WINDOW (manager->priv->volume_dialog), &orig_w, &orig_h);
        gtk_widget_get_preferred_size (manager->priv->volume_dialog, NULL, &win_req);

        if (win_req.width > orig_w) {
                orig_w = win_req.width;
        }
        if (win_req.height > orig_h) {
                orig_h = win_req.height;
        }
        orig_w = 64;
        orig_h = 300;
        gtk_window_set_default_size(GTK_WINDOW (manager->priv->volume_dialog), orig_w ,orig_h);
        pointer_screen = NULL;
        display = gdk_screen_get_display (manager->priv->current_screen);
        device_manager = gdk_display_get_device_manager (display);
        pointer = gdk_device_manager_get_client_pointer (device_manager);

        gdk_device_get_position (pointer,
                                 &pointer_screen,
                                 &pointer_x,
                                 &pointer_y);

        if (pointer_screen != manager->priv->current_screen) {
                /* The pointer isn't on the current screen, so just
                 * assume the default monitor
                 */
                monitor = 0;
        } else {
                monitor = gdk_screen_get_monitor_at_point (manager->priv->current_screen,
                                                           pointer_x,
                                                           pointer_y);
        }

        gdk_screen_get_monitor_geometry (manager->priv->current_screen,
                                         monitor,
                                         &geometry);

        screen_w = geometry.width;
        screen_h = geometry.height;

	    if(ukui_panel_position == 1){
                x = (screen_w * 0.01);
                y = ((screen_h - 300) - screen_h * 0.04);
        }else if(ukui_panel_position == 2){
                x = ((screen_w - 64) - screen_w * 0.01);
                y = (screen_h * 0.04);
        }else {
                x = (screen_w * 0.01);
                y = (screen_h * 0.04);
        }

        gtk_window_move (GTK_WINDOW (manager->priv->volume_dialog), x, y);

        GtkStyleContext * context = gtk_widget_get_style_context (manager->priv->volume_dialog);
        gtk_style_context_save (context);
        GtkCssProvider *provider = gtk_css_provider_new ();
        gtk_css_provider_load_from_data(provider, ".volume-box { border-radius:6px; background:rgba(19,20,20,0.95);}", -1, NULL);
        gtk_style_context_add_provider(context, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_USER);
        gtk_style_context_add_class (context, "volume-box");
        g_object_unref (provider);
        //gtk_render_background (context, cr, 20, 40, orig_w, _orig_h);
        //gtk_style_context_restore (context);

        gtk_widget_show (manager->priv->volume_dialog);

        gdk_display_sync (gdk_screen_get_display (manager->priv->current_screen));
        //gtk_style_context_restore (context);
}

static void
do_url_action (UsdMediaKeysManager *manager,
               const gchar         *scheme)
{
        GError *error = NULL;
        GAppInfo *app_info;

        app_info = g_app_info_get_default_for_uri_scheme (scheme);

        if (app_info != NULL) {
           if (!g_app_info_launch (app_info, NULL, NULL, &error)) {
                g_warning ("Could not launch '%s': %s",
                    g_app_info_get_commandline (app_info),
                    error->message);
		g_object_unref (app_info);
                g_error_free (error);
            }
        }
        else {
            g_warning ("Could not find default application for '%s' scheme", scheme);
        }
}

static void
do_media_action (UsdMediaKeysManager *manager)
{
        GError *error = NULL;
        GAppInfo *app_info;

        app_info = g_app_info_get_default_for_type ("audio/x-vorbis+ogg", FALSE);

        if (app_info != NULL) {
           if (!g_app_info_launch (app_info, NULL, NULL, &error)) {
                g_warning ("Could not launch '%s': %s",
                    g_app_info_get_commandline (app_info),
                    error->message);
                g_error_free (error);
            }
        }
        else {
            g_warning ("Could not find default application for '%s' mime-type", "audio/x-vorbis+ogg");
        }
}

static void
do_shutdown_action (UsdMediaKeysManager *manager)
{
        execute (manager, "ukui-session-tools", FALSE, FALSE);
}

static void
do_logout_action (UsdMediaKeysManager *manager)
{
        execute (manager, "ukui-session-tools", FALSE, FALSE);
}

static void
do_eject_action_cb (GDrive              *drive,
                    GAsyncResult        *res,
                    UsdMediaKeysManager *manager)
{
        g_drive_eject_with_operation_finish (drive, res, NULL);
}

#define NO_SCORE 0
#define SCORE_CAN_EJECT 50
#define SCORE_HAS_MEDIA 100
static void
do_eject_action (UsdMediaKeysManager *manager)
{
        GList *drives, *l;
        GDrive *fav_drive;
        guint score;

        /* Find the best drive to eject */
        fav_drive = NULL;
        score = NO_SCORE;
        drives = g_volume_monitor_get_connected_drives (manager->priv->volume_monitor);
        for (l = drives; l != NULL; l = l->next) {
                GDrive *drive = l->data;

                if (g_drive_can_eject (drive) == FALSE)
                        continue;
                if (g_drive_is_media_removable (drive) == FALSE)
                        continue;
                if (score < SCORE_CAN_EJECT) {
                        fav_drive = drive;
                        score = SCORE_CAN_EJECT;
                }
                if (g_drive_has_media (drive) == FALSE)
                        continue;
                if (score < SCORE_HAS_MEDIA) {
                        fav_drive = drive;
                        score = SCORE_HAS_MEDIA;
                        break;
                }
        }

        /* Show the dialogue */
        dialog_init (manager);
        usd_media_keys_window_set_action_custom (USD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                                 "media-eject",
                                                 FALSE);
        dialog_show (manager);

        /* Clean up the drive selection and exit if no suitable
         * drives are found */
        if (fav_drive != NULL)
                fav_drive = g_object_ref (fav_drive);

        g_list_foreach (drives, (GFunc) g_object_unref, NULL);
        if (fav_drive == NULL)
                return;

        /* Eject! */
        g_drive_eject_with_operation (fav_drive, G_MOUNT_UNMOUNT_FORCE,
                                      NULL, NULL,
                                      (GAsyncReadyCallback) do_eject_action_cb,
                                      manager);
        g_object_unref (fav_drive);
}

static void
do_touchpad_action (UsdMediaKeysManager *manager)
{
        GSettings *settings = g_settings_new (TOUCHPAD_SCHEMA);
        gboolean state = g_settings_get_boolean (settings, TOUCHPAD_ENABLED_KEY);
        if (touchpad_is_present () == FALSE) {
                dialog_init (manager);

                usd_media_keys_window_set_action_custom (USD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                                         "touchpad-disabled-symbolic", FALSE);
                usd_media_keys_window_set_action (USD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                          USD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM);
                return;
        }

        dialog_init (manager);
        usd_media_keys_window_set_action_custom (USD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                                 (!state) ? "touchpad-enabled-symbolic" : "touchpad-disabled-symbolic",
                                                 FALSE);
        usd_media_keys_window_set_action (USD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                          USD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM);
        dialog_show (manager);

        g_settings_set_boolean (settings, TOUCHPAD_ENABLED_KEY, !state);
        g_object_unref (settings);
}

#ifdef HAVE_LIBMATEMIXER
static void
update_dialog (UsdMediaKeysManager *manager,
               guint                volume,
               gboolean             muted,
               gboolean             sound_changed)
{
        volume_dialog_init (manager);

        usd_media_keys_window_set_volume_muted (USD_MEDIA_KEYS_WINDOW (manager->priv->volume_dialog),
                                                muted);
        usd_media_keys_window_set_volume_level (USD_MEDIA_KEYS_WINDOW (manager->priv->volume_dialog),
                                                volume);

        usd_media_keys_window_set_action (USD_MEDIA_KEYS_WINDOW (manager->priv->volume_dialog),
                                          USD_MEDIA_KEYS_WINDOW_ACTION_VOLUME);
        volume_dialog_show (manager);

#ifdef HAVE_LIBCANBERRA
        if (sound_changed != FALSE && muted == FALSE)
                ca_gtk_play_for_widget (manager->priv->volume_dialog, 0,
                                        CA_PROP_EVENT_ID, "audio-volume-change",
                                        CA_PROP_EVENT_DESCRIPTION, "Volume changed through key press",
                                        CA_PROP_APPLICATION_NAME, PACKAGE_NAME,
                                        CA_PROP_APPLICATION_VERSION, PACKAGE_VERSION,
                                        CA_PROP_APPLICATION_ID, "org.ukui.SettingsDaemon",
                                        NULL);
#endif
}

static void
do_sound_action (UsdMediaKeysManager *manager, int type)
{
        gboolean muted;
        gboolean muted_last;
        gboolean sound_changed = FALSE;
        guint    volume;
        // guint    volume_min, volume_max;
		guint    volume_min, volume_max, maxlessmin;
        guint    volume_step;
        guint    volume_last;

        if (manager->priv->control == NULL)
                return;

        /* Theoretically the volume limits might be different for different
         * streams, also the minimum might not always start at 0 */
        volume_min = mate_mixer_stream_control_get_min_volume (manager->priv->control);
        volume_max = mate_mixer_stream_control_get_normal_volume (manager->priv->control);
        maxlessmin = volume_max - volume_min;
        volume_step = g_settings_get_int (manager->priv->settings, "volume-step");
        if (volume_step <= 0 ||
            volume_step > 100)
                volume_step = VOLUME_STEP;

        /* Scale the volume step size accordingly to the range used by the control */
        volume_step = (volume_max - volume_min) * volume_step / 100;

        volume = volume_last =
                mate_mixer_stream_control_get_volume (manager->priv->control);
        muted = muted_last =
                mate_mixer_stream_control_get_mute (manager->priv->control);

        //获取底层的声音将其转换为界面的声音，再做递增/减，递增递减之后再将其显示到界面上，最后再转换写入到底层
        /*if(volume <= volume_min + maxlessmin / 100 * 60.0)
            volume = volume_min + (volume - volume_min) / 3.0;
        else if(volume > volume_min + maxlessmin / 100.0 * 80.0)
            volume = volume_min + maxlessmin / 100.0 * 40.0 +((volume - volume_min) - maxlessmin / 100.0 * 80.0) * 3.0;
        else
            volume = volume - maxlessmin / 100.0 * 40.0;
        */
        switch (type) {
        case MUTE_KEY:
                muted = !muted;
                break;
        case VOLUME_DOWN_KEY:
                if (volume <= (volume_min + volume_step)) {
                        volume = volume_min;
                        muted  = TRUE;
                } else {
                        volume -= volume_step;
                        muted  = FALSE;
                }
                break;
        case VOLUME_UP_KEY:
                if (muted) {
                        muted = FALSE;
                        if (volume <= volume_min)
                               volume = volume_min + volume_step;
                } else
                        volume = CLAMP (volume + volume_step,
                                        volume_min,
                                        volume_max);
                break;
        }

        if (muted != muted_last) {
                if (mate_mixer_stream_control_set_mute (manager->priv->control, muted))
                        sound_changed = TRUE;
                else
                        muted = muted_last;
        }

        update_dialog (manager,
                       CLAMP (100 * volume / (volume_max - volume_min), 0, 100),
                       muted,
                       sound_changed);
        /*
        if(volume <= volume_min + maxlessmin / 100 * 20.0)
            volume = volume_min + (volume - volume_min) * 3.0;
        else if(volume > volume_min + maxlessmin / 100.0 * 40.0)
            volume = volume_min + maxlessmin / 100.0 * 80.0 + ((volume - volume_min) - maxlessmin / 100.0 * 40.0) / 3.0;
        else
            volume = volume + maxlessmin / 100.0 * 40.0;
        */
        if (volume != mate_mixer_stream_control_get_volume (manager->priv->control)) {
                if (mate_mixer_stream_control_set_volume (manager->priv->control, volume))
                        sound_changed = TRUE;
                else
                        volume = volume_last;
        }
        /*
        update_dialog (manager,
                       CLAMP (100 * volume / (volume_max - volume_min), 0, 100),
                       muted,
                       sound_changed);
        */
}
static void
do_mic_sound_action (UsdMediaKeysManager *manager)
{
        gboolean mute;
        mute = mate_mixer_stream_control_get_mute (manager->priv->input_control);
        mate_mixer_stream_control_set_mute(manager->priv->input_control, !mute);
        dialog_init (manager);
        usd_media_keys_window_set_action_custom (USD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                                 mute ? "audio-input-microphone-high-symbolic" : "audio-input-microphone-muted-symbolic", FALSE);
        usd_media_keys_window_set_action (USD_MEDIA_KEYS_WINDOW (manager->priv->dialog),
                                          USD_MEDIA_KEYS_WINDOW_ACTION_CUSTOM);
        dialog_show (manager);
}

static void
update_default_input (UsdMediaKeysManager *manager)
{
        MateMixerStream        *input_stream;
        MateMixerStreamControl *input_control = NULL;

        input_stream = mate_mixer_context_get_default_input_stream (manager->priv->context);
        if (input_stream != NULL)
              input_control = mate_mixer_stream_get_default_control (input_stream);
        if (input_stream == manager->priv->input_stream)
              return;

        g_clear_object (&manager->priv->input_stream);
        g_clear_object (&manager->priv->input_control);

        if (input_control != NULL){
            MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags (input_control);

            /* Do not use the stream if it is not possible to mute it or
             * change the volume */
            if (!(flags & MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE) &&
                !(flags & MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE))
                    return;
            manager->priv->input_stream  = g_object_ref (input_stream);
            manager->priv->input_control = g_object_ref (input_control);
            g_debug ("Default output stream updated to %s",
                         mate_mixer_stream_get_name (input_stream));
        } else
              g_debug ("Default output stream unset");
}

static void
update_default_output (UsdMediaKeysManager *manager)
{
        MateMixerStream        *stream;
        MateMixerStreamControl *control = NULL;

        stream = mate_mixer_context_get_default_output_stream (manager->priv->context);
        if (stream != NULL)
                control = mate_mixer_stream_get_default_control (stream);

        if (stream == manager->priv->stream)
                return;

        g_clear_object (&manager->priv->stream);
        g_clear_object (&manager->priv->control);

        if (control != NULL) {
                MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags (control);

                /* Do not use the stream if it is not possible to mute it or
                 * change the volume */
                if (!(flags & MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE) &&
                    !(flags & MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE))
                        return;

                manager->priv->stream  = g_object_ref (stream);
                manager->priv->control = g_object_ref (control);
                g_debug ("Default output stream updated to %s",
                         mate_mixer_stream_get_name (stream));
        } else
                g_debug ("Default output stream unset");
}

static void
on_context_state_notify (MateMixerContext    *context,
                         GParamSpec          *pspec,
                         UsdMediaKeysManager *manager)
{
        update_default_input (manager);
        update_default_output (manager);
}

static void
on_context_default_input_notify (MateMixerContext    *context,
                                 GParamSpec          *pspec,
                                 UsdMediaKeysManager *manager)
{
        update_default_input (manager);
}

static void
on_context_default_output_notify (MateMixerContext    *context,
                                  GParamSpec          *pspec,
                                  UsdMediaKeysManager *manager)
{
        update_default_output (manager);
}

static void
on_context_stream_removed (MateMixerContext    *context,
                           const gchar         *name,
                           UsdMediaKeysManager *manager)
{
        if (manager->priv->stream != NULL) {
                MateMixerStream *stream =
                        mate_mixer_context_get_stream (manager->priv->context, name);

                if (stream == manager->priv->stream) {
                        g_clear_object (&manager->priv->stream);
                        g_clear_object (&manager->priv->control);
                }
        }
}
#endif /* HAVE_LIBMATEMIXER */

static gint
find_by_application (gconstpointer a,
                     gconstpointer b)
{
        return strcmp (((MediaPlayer *)a)->application, b);
}

static gint
find_by_time (gconstpointer a,
              gconstpointer b)
{
        return ((MediaPlayer *)a)->time < ((MediaPlayer *)b)->time;
}

/*
 * Register a new media player. Most applications will want to call
 * this with time = GDK_CURRENT_TIME. This way, the last registered
 * player will receive media events. In some cases, applications
 * may want to register with a lower priority (usually 1), to grab
 * events only nobody is interested.
 */
gboolean
usd_media_keys_manager_grab_media_player_keys (UsdMediaKeysManager *manager,
                                               const char          *application,
                                               guint32              time,
                                               GError             **error)
{
        GList       *iter;
        MediaPlayer *media_player;

        if (time == GDK_CURRENT_TIME) {
                GTimeVal tv;

                g_get_current_time (&tv);
                time = tv.tv_sec * 1000 + tv.tv_usec / 1000;
        }

        iter = g_list_find_custom (manager->priv->media_players,
                                   application,
                                   find_by_application);

        if (iter != NULL) {
                if (((MediaPlayer *)iter->data)->time < time) {
                        g_free (((MediaPlayer *)iter->data)->application);
                        g_free (iter->data);
                        manager->priv->media_players = g_list_delete_link (manager->priv->media_players, iter);
                } else {
                        return TRUE;
                }
        }

        g_debug ("Registering %s at %u", application, time);
        media_player = g_new0 (MediaPlayer, 1);
        media_player->application = g_strdup (application);
        media_player->time = time;

        manager->priv->media_players = g_list_insert_sorted (manager->priv->media_players,
                                                             media_player,
                                                             find_by_time);

        return TRUE;
}

gboolean
usd_media_keys_manager_release_media_player_keys (UsdMediaKeysManager *manager,
                                                  const char          *application,
                                                  GError             **error)
{
        GList *iter;

        iter = g_list_find_custom (manager->priv->media_players,
                                   application,
                                   find_by_application);

        if (iter != NULL) {
                g_debug ("Deregistering %s", application);
                g_free (((MediaPlayer *)iter->data)->application);
                g_free (iter->data);
                manager->priv->media_players = g_list_delete_link (manager->priv->media_players, iter);
        }

        return TRUE;
}

static gboolean
usd_media_player_key_pressed (UsdMediaKeysManager *manager,
                              const char          *key)
{
        const char *application = NULL;
        gboolean    have_listeners;

        have_listeners = (manager->priv->media_players != NULL);

        if (have_listeners) {
                application = ((MediaPlayer *)manager->priv->media_players->data)->application;
        }

        g_signal_emit (manager, signals[MEDIA_PLAYER_KEY_PRESSED], 0, application, key);

        return !have_listeners;
}

static gboolean
do_multimedia_player_action (UsdMediaKeysManager *manager,
                             const char          *key)
{
        return usd_media_player_key_pressed (manager, key);
}

static void
do_toggle_accessibility_key (const char *key)
{
        GSettings *settings;
        gboolean state;

        settings = g_settings_new ("org.gnome.desktop.a11y.applications");
        state = g_settings_get_boolean (settings, key);
        g_settings_set_boolean (settings, key, !state);
        g_object_unref (settings);
}

static void
do_magnifier_action (UsdMediaKeysManager *manager)
{
        do_toggle_accessibility_key ("screen-magnifier-enabled");
}

static void
do_screenreader_action (UsdMediaKeysManager *manager)
{
        do_toggle_accessibility_key ("screen-reader-enabled");
}

static void
do_on_screen_keyboard_action (UsdMediaKeysManager *manager)
{
        do_toggle_accessibility_key ("screen-keyboard-enabled");
}

static void
do_terminal_action (UsdMediaKeysManager *manager)
{
        execute (manager, "mate-terminal",FALSE,FALSE);
}

static void
do_screenshot_action (UsdMediaKeysManager *manager)
{
        execute (manager, "kylin-screenshot full",FALSE,FALSE);
}

static void
do_area_screenshot_action (UsdMediaKeysManager *manager)
{
        execute (manager, "kylin-screenshot gui",FALSE,FALSE);
}

static void
do_window_screenshot_action (UsdMediaKeysManager *manager)
{
        execute (manager, "kylin-screenshot screen",FALSE,FALSE);
}

static void
do_window_switch_action (UsdMediaKeysManager *manager)
{
        execute (manager, "ukui-window-switch --show-workspace",FALSE,FALSE);
}

static void
do_system_monitor_action (UsdMediaKeysManager *manager)
{
        execute (manager, "ukui-system-monitor",FALSE,FALSE);
}

static void
do_connection_editor_action (UsdMediaKeysManager *manager)
{
         execute (manager, "nm-connection-editor",FALSE,FALSE);
}

static void
do_open_ukui_search_action(UsdMediaKeysManager *manager)
{
    GError *gerr = NULL;

    GDBusConnection *con = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
    g_dbus_connection_call_sync (con,
                                 "com.ukui.search.service",
                                 "/",
                                 "org.ukui.search.service",
                                 "showWindow",
                                 NULL,
                                 NULL,
                                 G_DBUS_CALL_FLAGS_NONE,
                                 -1,
                                 NULL,
                                 &gerr);
    if (gerr) {
        execute (manager, "ukui-search -s",FALSE,FALSE);
        g_error_free (gerr);
    }
    g_object_unref (con);

}

static void
do_open_kds_action(UsdMediaKeysManager *manager)
{
         execute (manager, "kydisplayswitch",FALSE,FALSE);
}

static gboolean
do_action (UsdMediaKeysManager *manager,
           int                  type)
{
        char *cmd;
        char *path;

        gboolean shot_res = g_settings_get_boolean (manager->priv->screenshot_settings, RUNINGS_KEY);
        if(shot_res)
            return FALSE;

        switch (type) {
        case TOUCHPAD_KEY:
                do_touchpad_action (manager);
                break;
        case MUTE_KEY:
        case VOLUME_DOWN_KEY:
        case VOLUME_UP_KEY:
#ifdef HAVE_LIBMATEMIXER
                do_sound_action (manager, type);
#endif
                break;
        case MIC_MUTE_KEY:
                do_mic_sound_action (manager);
                break;
        case POWER_KEY:
                do_shutdown_action (manager);
                break;
	case LOGOUT_KEY:
	case LOGOUT_KEY1:
	case LOGOUT_KEY2:
		do_logout_action (manager);
		break;
        case EJECT_KEY:
                do_eject_action (manager);
                break;
        case HOME_KEY:
                path = g_shell_quote (g_get_home_dir ());
                cmd = g_strconcat ("caja --no-desktop ", path, NULL);
                g_free (path);
                execute (manager, cmd, FALSE, FALSE);
                g_free (cmd);
                break;
        case SEARCH_KEY:
                cmd = NULL;
                if ((cmd = g_find_program_in_path ("beagle-search"))) {
                        execute (manager, "beagle-search", FALSE, FALSE);
                } else if ((cmd = g_find_program_in_path ("tracker-search-tool"))) {
                        execute (manager, "tracker-search-tool", FALSE, FALSE);
                } else {
                        execute (manager, "mate-search-tool", FALSE, FALSE);
                }
                g_free (cmd);
                break;
        case EMAIL_KEY:
                do_url_action (manager, "mailto");
                break;
        case SCREENSAVER_KEY:
        case SCREENSAVER_KEY_2:
                if ((cmd = g_find_program_in_path ("ukui-screensaver-command"))) {
                        execute (manager, "ukui-screensaver-command --lock", FALSE, FALSE);
                } else if ((cmd = g_find_program_in_path ("ukui-screensaver-command"))) {
                        execute (manager, "ukui-screensaver-command -l" ,FALSE, FALSE);
                } else {
                        execute (manager, "xscreensaver-command -lock", FALSE, FALSE);
                }
                g_free (cmd);
                break;
        case SETTINGS_KEY:
        case SETTINGS_KEY_2:
                execute(manager, "ukui-control-center", FALSE, FALSE);
                break;
        case FILE_MANAGER_KEY:
        case FILE_MANAGER_KEY_2:
                execute(manager, "peony", FALSE, FALSE);
                break;
        case HELP_KEY:
                do_url_action (manager, "help");
                break;
        case WWW_KEY:
                do_url_action (manager, "http");
                break;
        case MEDIA_KEY:
                do_media_action (manager);
                break;
        case CALCULATOR_KEY:
                if ((cmd = g_find_program_in_path ("galculator"))) {
                        execute (manager, "galculator", FALSE, FALSE);
                } else if ((cmd = g_find_program_in_path ("ukui-calc"))) {
                        execute (manager, "ukui-calc", FALSE, FALSE);
                } else {
                        execute (manager, "kylin-calculator", FALSE, FALSE);
                }

                g_free (cmd);
                break;
        case PLAY_KEY:
                return do_multimedia_player_action (manager, "Play");
        case PAUSE_KEY:
                return do_multimedia_player_action (manager, "Pause");
        case STOP_KEY:
                return do_multimedia_player_action (manager, "Stop");
        case PREVIOUS_KEY:
                return do_multimedia_player_action (manager, "Previous");
        case NEXT_KEY:
                return do_multimedia_player_action (manager, "Next");
        case REWIND_KEY:
                return do_multimedia_player_action (manager, "Rewind");
        case FORWARD_KEY:
                return do_multimedia_player_action (manager, "FastForward");
        case REPEAT_KEY:
                return do_multimedia_player_action (manager, "Repeat");
        case RANDOM_KEY:
                return do_multimedia_player_action (manager, "Shuffle");
        case MAGNIFIER_KEY:
                do_magnifier_action (manager);
                break;
        case SCREENREADER_KEY:
                do_screenreader_action (manager);
                break;
        case ON_SCREEN_KEYBOARD_KEY:
                do_on_screen_keyboard_action (manager);
                break;
        case TERMINAL_KEY:
        case TERMINAL_KEY_2:
                do_terminal_action (manager);
                break;
        case SCREENSHOT_KEY:
                do_screenshot_action (manager);
                break;
        case AREA_SCREENSHOT_KEY:
                do_area_screenshot_action (manager);
                break;
        case WINDOW_SCREENSHOT_KEY:
                do_window_screenshot_action (manager);
                break;
        case WINDOWSWITCH_KEY:
        case WINDOWSWITCH_KEY_2:
                do_window_switch_action (manager);
                break;
        case SYSTEM_MONITOR_KEY:
                do_system_monitor_action (manager);
                break;
        case CONNECTION_EDITOR_KEY:
                do_connection_editor_action (manager);
                break;
        case GLOBAL_SEARCH_KEY:
                do_open_ukui_search_action (manager);
                break;
        case KDS_KEY:
        case KDS_KEY2:
                do_open_kds_action (manager);
                break;
        default:
                g_assert_not_reached ();
        }

        return FALSE;
}

static GdkScreen *
acme_get_screen_from_event (UsdMediaKeysManager *manager,
                            XAnyEvent           *xanyev)
{
        GdkWindow *window;
        GdkScreen *screen;
        GSList    *l;

        /* Look for which screen we're receiving events */
        for (l = manager->priv->screens; l != NULL; l = l->next) {
                screen = (GdkScreen *) l->data;
                window = gdk_screen_get_root_window (screen);

                if (GDK_WINDOW_XID (window) == xanyev->window) {
                        return screen;
                }
        }

        return NULL;
}

static GdkFilterReturn
acme_filter_events (GdkXEvent           *xevent,
                    GdkEvent            *event,
                    UsdMediaKeysManager *manager)
{
        XEvent     *xev    = (XEvent *) xevent;
        XAnyEvent  *xany   = (XAnyEvent *) xevent;
        int        i;

        /* verify we have a key event */
        if (xev->type != KeyPress && xev->type != KeyRelease) {
                return GDK_FILTER_CONTINUE;
        }
        /*if (xev->type == KeyRelease &&xev->xkey.keycode == 133) {
		    system("ukui-menu");
		}
        */
        for (i = 0; i < HANDLED_KEYS; i++) {
                if (match_key (keys[i].key, xev)) {
                        switch (keys[i].key_type) {
                        case VOLUME_DOWN_KEY:
                        case VOLUME_UP_KEY:
                                /* auto-repeatable keys */
                                if (xev->type != KeyPress) {
                                        return GDK_FILTER_CONTINUE;
                                }
                                break;
                        default:
                                if (xev->type != KeyRelease) {
                                        return GDK_FILTER_CONTINUE;
                                }
                        }

                        manager->priv->current_screen = acme_get_screen_from_event (manager, xany);

                        if (do_action (manager, keys[i].key_type) == FALSE) {
                                return GDK_FILTER_REMOVE;
                        } else {
                                return GDK_FILTER_CONTINUE;
                        }
                }
        }

        return GDK_FILTER_CONTINUE;
}

static gboolean m_CtrlFlag = FALSE;

void key_release_str (UsdMediaKeysManager *manager,
                      char *key_str)
{
    static gboolean ctrlFlag = FALSE;
    if (g_strcmp0(key_str, "Shift_L+Print")==0 ||
        g_strcmp0(key_str, "Shift_R+Print")==0 ){
        do_area_screenshot_action(manager);
        return;
    }

    if(g_strcmp0(key_str, "Print")==0){
          do_screenshot_action(manager);
          return;
    }

    if(g_strcmp0(key_str, "Control_L+Shift_L+Escape")==0){
          do_system_monitor_action (manager);
          return;
    }

    if (strncmp(key_str, "Control_L+", 10) == 0 ||
        strncmp(key_str, "Control_R+", 10) == 0 )
        ctrlFlag = TRUE;

    if(ctrlFlag && g_strcmp0(key_str, "Control_L") == 0 ||
       ctrlFlag && g_strcmp0(key_str, "Control_R") == 0 ){
        ctrlFlag = FALSE;
        return;
    } else if (m_CtrlFlag && g_strcmp0(key_str, "Control_L") == 0 ||
               m_CtrlFlag && g_strcmp0(key_str, "Control_R") == 0 )
        return;

    if( g_strcmp0 (key_str, "Control_L") == 0 ||
        g_strcmp0 (key_str, "Control_R") == 0 )
    {
        g_settings_set_boolean (manager->priv->point_settings,
                                POINTER_KEY,
                                (!g_settings_get_boolean(manager->priv->point_settings,
                                                         POINTER_KEY)));
    }

}

void key_press_str (char *key_str)
{
    if(strncmp(key_str, "Control_L+", 10) == 0 ||
       strncmp(key_str, "Control_R+", 10) == 0 )
        m_CtrlFlag = TRUE;

    if(m_CtrlFlag && g_strcmp0(key_str, "Control_L") == 0 ||
       m_CtrlFlag && g_strcmp0(key_str, "Control_R") == 0 )
        m_CtrlFlag = FALSE;
}

void KeyReleaseModifier(UsdMediaKeysManager *manager,
                        xEvent              *event)
{

    GList *list, *l;
    Display *display = XOpenDisplay(NULL);
    char *KeyStr = malloc (256);
    char *Str    = malloc (256);
    KeySym keySym = XkbKeycodeToKeysym(display, event->u.u.detail, 0, 0);
    int hashNum = g_hash_table_size (manager->priv->hash);

    memset (KeyStr, 0, sizeof(KeyStr));
    memset (Str,    0, sizeof(Str));
    if(hashNum != 0){
        list = g_hash_table_get_keys (manager->priv->hash);

        for(l = list; l!=NULL;l = l->next){
            strcat(Str, XKeysymToString(l->data));
            strcat (Str, "+");
        }
        for(int i=0; i<8; i++){
            if(ModifiersVec[i] == keySym){
                Str[strlen(Str) - 1] = '\0';
                strcpy(KeyStr, Str);
                goto END;
            }
        }
        strcat (KeyStr, Str);
    }
    strcat (KeyStr, XKeysymToString(keySym));

END :
    key_release_str (manager, KeyStr);
    free (KeyStr);
    free(Str);
    XCloseDisplay(display);

}

void KeyPressModifier(UsdMediaKeysManager *manager,
                      xEvent              *event)
{

    GList *list, *l;
    Display *display = XOpenDisplay(NULL);
    char *KeyStr = malloc (256);
    char *Str    = malloc (256);
    int hashNum = g_hash_table_size (manager->priv->hash);
    KeySym keySym = XkbKeycodeToKeysym(display, event->u.u.detail, 0, 0);

    memset (KeyStr, 0, sizeof(KeyStr));
    memset (Str,    0, sizeof(Str));
    list = g_hash_table_get_keys (manager->priv->hash);
    for(l = list; l!=NULL;l = l->next){
        strcat(Str, XKeysymToString(l->data));
        strcat (Str, "+");
    }
    for(int i=0; i<8; i++){
        if(ModifiersVec[i] == keySym){
            Str[strlen(Str) - 1] = '\0';
            strcpy(KeyStr, Str);
            goto END;
        }
    }
    strcat (KeyStr, Str);
    strcat (KeyStr, XKeysymToString(keySym));

END:
    key_press_str (KeyStr);
    free(KeyStr);
    free(Str);
    XCloseDisplay(display);
}

void updateModifier(xEvent *event, 
                    gboolean isAdd,
                    UsdMediaKeysManager *manager)
{
    Display *display = XOpenDisplay(NULL);
    KeySym keySym = XkbKeycodeToKeysym(display, event->u.u.detail, 0, 0);

    for(int i=0; i<8; i++){
        if(ModifiersVec[i] == keySym){
            if (isAdd)
                g_hash_table_add (manager->priv->hash, keySym);
            else if (g_hash_table_contains(manager->priv->hash, keySym))
                g_hash_table_remove (manager->priv->hash, keySym);
        }
    }
    XCloseDisplay(display);
}

static void
handleRecordEvent(UsdMediaKeysManager *manager,
                  XRecordInterceptData* data)
{
    if (data->category == XRecordFromServer) {
        xEvent * event = (xEvent *)data->data;
        switch (event->u.u.type)
        {
        case KeyPress:
             updateModifier (event, TRUE, manager);
             KeyPressModifier(manager, event);
             break;
        case KeyRelease:
            updateModifier (event, FALSE, manager);
            KeyReleaseModifier(manager, event);
            break;
        default:
            break;
        }
    }
}

static void
xevent_callback(XPointer ptr,
                XRecordInterceptData* data)
{
    handleRecordEvent((UsdMediaKeysManager *)ptr, data);
}

static void
xevent_monitor(UsdMediaKeysManager *manager)
{
    Display* display = XOpenDisplay(0);
    if (display == 0) {
        fprintf(stderr, "unable to open display\n");
        return;
    }
    // Receive from ALL clients, including future clients.
    XRecordClientSpec clients = XRecordAllClients;
    XRecordRange* range = XRecordAllocRange();
    if (range == 0) {
        fprintf(stderr, "unable to allocate XRecordRange\n");
        return;
    }
    memset(range, 0, sizeof(XRecordRange));
    range->device_events.first = KeyPress;
    range->device_events.last  = KeyRelease;

    // And create the XRECORD context.
    XRecordContext context = XRecordCreateContext(display, 0, &clients, 1, &range, 1);
    if (context == 0) {
        fprintf(stderr, "XRecordCreateContext failed\n");
        return;
    }
    XFree(range);
    XSync(display, True);

    Display* display_datalink = XOpenDisplay(0);
    if (display_datalink == 0) {
        fprintf(stderr, "unable to open second display\n");
        XCloseDisplay (display_datalink);
        return;
    }
    if (!XRecordEnableContextAsync(display_datalink, context,  xevent_callback, manager)) {
        fprintf(stderr, "XRecordEnableContext() failed\n");
        XCloseDisplay (display_datalink);
        return;
    }
    XCloseDisplay (display_datalink);
}

static void
init_xevent_monitor(UsdMediaKeysManager *manager)
{
    manager->priv->hash = g_hash_table_new(NULL, NULL);

    int pthr_xevent = -1;
    pthread_t thread;
    pthr_xevent = pthread_create(&thread, NULL, (void *)&xevent_monitor, (void*)manager);
    if(pthr_xevent != 0)
    {
        printf("[%s%d] creat thread failed\n", __FUNCTION__, __LINE__);
    }
}

static gboolean
start_media_keys_idle_cb (UsdMediaKeysManager *manager)
{
        GSList *l;

        g_debug ("Starting media_keys manager");
        ukui_settings_profile_start (NULL);
        manager->priv->volume_monitor = g_volume_monitor_get ();
        manager->priv->settings = g_settings_new (BINDING_SCHEMA);
        manager->priv->point_settings = g_settings_new (POINTER_SCHEMA);
        manager->priv->session_settings = g_settings_new (SESSION_SCHEMA);
        manager->priv->screenshot_settings = g_settings_new (SCREENSHOT_SCHEMA);
        g_settings_set_boolean (manager->priv->screenshot_settings, RUNINGS_KEY, FALSE);

        init_screens (manager);
        init_kbd (manager);
        init_xevent_monitor (manager);
        /* Start filtering the events */
        for (l = manager->priv->screens; l != NULL; l = l->next) {
                ukui_settings_profile_start ("gdk_window_add_filter");

                g_debug ("adding key filter for screen: %d",
                         gdk_screen_get_number (l->data));

                gdk_window_add_filter (gdk_screen_get_root_window (l->data),
                                       (GdkFilterFunc)acme_filter_events,
                                       manager);
                ukui_settings_profile_end ("gdk_window_add_filter");
        }

        ukui_settings_profile_end (NULL);

        return FALSE;
}

gboolean
usd_media_keys_manager_start (UsdMediaKeysManager *manager, GError **error)
{
        ukui_settings_profile_start (NULL);

#ifdef HAVE_LIBMATEMIXER
        if (G_LIKELY (mate_mixer_is_initialized ())) {
                ukui_settings_profile_start ("mate_mixer_context_new");

                manager->priv->context = mate_mixer_context_new ();

                g_signal_connect (manager->priv->context,
                                  "notify::state",
                                  G_CALLBACK (on_context_state_notify),
                                  manager);
                g_signal_connect (manager->priv->context,
                                  "notify::default-input-stream",
                                  G_CALLBACK (on_context_default_input_notify),
                                  manager);
                g_signal_connect (manager->priv->context,
                                  "notify::default-output-stream",
                                  G_CALLBACK (on_context_default_output_notify),
                                  manager);
                g_signal_connect (manager->priv->context,
                                  "stream-removed",
                                  G_CALLBACK (on_context_stream_removed),
                                  manager);

                mate_mixer_context_open (manager->priv->context);

                ukui_settings_profile_end ("mate_mixer_context_new");
        }
#endif
        g_idle_add ((GSourceFunc) start_media_keys_idle_cb, manager);

        ukui_settings_profile_end (NULL);

        return TRUE;
}

void
usd_media_keys_manager_stop (UsdMediaKeysManager *manager)
{
        UsdMediaKeysManagerPrivate *priv = manager->priv;
        GSList *ls;
        GList *l;
        int i;
        gboolean need_flush;

        g_debug ("Stopping media_keys manager");

        for (ls = priv->screens; ls != NULL; ls = ls->next) {
                gdk_window_remove_filter (gdk_screen_get_root_window (ls->data),
                                          (GdkFilterFunc) acme_filter_events,
                                          manager);
        }

        if (priv->settings != NULL) {
                g_object_unref (priv->settings);
                priv->settings = NULL;
        }
        if (priv->point_settings != NULL) {
                g_object_unref (priv->point_settings);
                priv->point_settings = NULL;
        }
        if (priv->session_settings != NULL) {
                g_object_unref (priv->session_settings);
                priv->session_settings = NULL;
        }
        if (priv->screenshot_settings != NULL) {
                g_object_unref (priv->screenshot_settings);
                priv->screenshot_settings = NULL;
        }
        if (priv->volume_monitor != NULL) {
                g_object_unref (priv->volume_monitor);
                priv->volume_monitor = NULL;
        }

        if (priv->connection != NULL) {
                dbus_g_connection_unref (priv->connection);
                priv->connection = NULL;
        }

        need_flush = FALSE;
        gdk_x11_display_error_trap_push (gdk_display_get_default ());

        for (i = 0; i < HANDLED_KEYS; ++i) {
                if (keys[i].key) {
                        need_flush = TRUE;
                        grab_key_unsafe (keys[i].key, FALSE, priv->screens);

                        g_free (keys[i].key->keycodes);
                        g_free (keys[i].key);
                        keys[i].key = NULL;
                }
        }

        if (need_flush)
                gdk_flush ();

        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default ());

        g_slist_free (priv->screens);
        priv->screens = NULL;

#ifdef HAVE_LIBMATEMIXER
        g_clear_object (&priv->stream);
        g_clear_object (&priv->control);
        g_clear_object (&priv->context);
#endif

        if (priv->dialog != NULL) {
                gtk_widget_destroy (priv->dialog);
                priv->dialog = NULL;
        }
        if (priv->volume_dialog != NULL) {
                gtk_widget_destroy (priv->volume_dialog);
                priv->volume_dialog = NULL;
        }
        for (l = priv->media_players; l; l = l->next) {
                MediaPlayer *mp = l->data;
                g_free (mp->application);
                g_free (mp);
        }
        g_list_free (priv->media_players);
        priv->media_players = NULL;
}

static void
usd_media_keys_manager_class_init (UsdMediaKeysManagerClass *klass)
{
        signals[MEDIA_PLAYER_KEY_PRESSED] =
                g_signal_new ("media-player-key-pressed",
                              G_OBJECT_CLASS_TYPE (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (UsdMediaKeysManagerClass, media_player_key_pressed),
                              NULL,
                              NULL,
                              usd_marshal_VOID__STRING_STRING,
                              G_TYPE_NONE,
                              2,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

        dbus_g_object_type_install_info (USD_TYPE_MEDIA_KEYS_MANAGER, &dbus_glib_usd_media_keys_manager_object_info);

        g_type_class_add_private (klass, sizeof (UsdMediaKeysManagerPrivate));
}

static void
usd_media_keys_manager_init (UsdMediaKeysManager *manager)
{
        manager->priv = USD_MEDIA_KEYS_MANAGER_GET_PRIVATE (manager);
        manager->priv->bcon = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
        manager->priv->bproxy = g_dbus_proxy_new_sync (manager->priv->bcon, G_DBUS_PROXY_FLAGS_NONE, NULL, "com.ukui.panel.desktop", "/", "com.ukui.panel.desktop", NULL, NULL);
}

static gboolean
register_manager (UsdMediaKeysManager *manager)
{
        GError *error = NULL;

        manager->priv->connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (manager->priv->connection == NULL) {
                if (error != NULL) {
                        g_error ("Error getting session bus: %s", error->message);
                        g_error_free (error);
                }
                return FALSE;
        }

        dbus_g_connection_register_g_object (manager->priv->connection, USD_MEDIA_KEYS_DBUS_PATH, G_OBJECT (manager));

        return TRUE;
}

UsdMediaKeysManager *
usd_media_keys_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                gboolean res;

                manager_object = g_object_new (USD_TYPE_MEDIA_KEYS_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
                res = register_manager (manager_object);
                if (! res) {
                        g_object_unref (manager_object);
                        return NULL;
                }
        }

        return USD_MEDIA_KEYS_MANAGER (manager_object);
}
