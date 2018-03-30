/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2001 Ximian, Inc.
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2007 Red Hat, Inc.
 * Copyright (C) 2012 Jasmine Hassan <jasmine.aura@gmail.com>
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

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gio/gio.h>

#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-bg.h>
#include <X11/Xatom.h>

#include "ukui-settings-profile.h"
#include "usd-background-manager.h"

#define UKUI_SESSION_MANAGER_DBUS_NAME "org.gnome.SessionManager"
#define UKUI_SESSION_MANAGER_DBUS_PATH "/org/gnome/SessionManager"

#define USD_BACKGROUND_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), USD_TYPE_BACKGROUND_MANAGER, UsdBackgroundManagerPrivate))

struct UsdBackgroundManagerPrivate {
	GSettings       *settings;
	MateBG          *bg;
	cairo_surface_t *surface;
	MateBGCrossfade *fade;
	GList           *scr_sizes;

	gboolean         usd_can_draw;
	gboolean         peony_can_draw;
	gboolean         do_fade;
	gboolean         draw_in_progress;

	guint            timeout_id;

	GDBusProxy      *proxy;
	guint            proxy_signal_id;
};

G_DEFINE_TYPE (UsdBackgroundManager, usd_background_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

/* Whether USD is allowed to draw background */
static gboolean
usd_can_draw_bg (UsdBackgroundManager *manager)
{
	return g_settings_get_boolean (manager->priv->settings, MATE_BG_KEY_DRAW_BACKGROUND);
}

/* Whether to change background with a fade effect */
static gboolean
can_fade_bg (UsdBackgroundManager *manager)
{
	return g_settings_get_boolean (manager->priv->settings, MATE_BG_KEY_BACKGROUND_FADE);
}

/* Whether Peony is configured to draw desktop (show-desktop-icons) */
static gboolean
peony_can_draw_bg (UsdBackgroundManager *manager)
{
	return g_settings_get_boolean (manager->priv->settings, MATE_BG_KEY_SHOW_DESKTOP);
}

static gboolean
peony_is_drawing_bg (UsdBackgroundManager *manager)
{
	Display       *display = gdk_x11_get_default_xdisplay ();
	Window         window = gdk_x11_get_default_root_xwindow ();
	Atom           peony_prop, wmclass_prop, type;
	Window         peony_window;
	int            format;
	unsigned long  nitems, after;
	unsigned char *data;
	gboolean       running = FALSE;

	if (!manager->priv->peony_can_draw)
		return FALSE;

	peony_prop = XInternAtom (display, "PEONY_DESKTOP_WINDOW_ID", True);
	if (peony_prop == None)
		return FALSE;

	XGetWindowProperty (display, window, peony_prop, 0, 1, False,
			    XA_WINDOW, &type, &format, &nitems, &after, &data);

	if (data == NULL)
		return FALSE;

	peony_window = *(Window *) data;
	XFree (data);

	if (type != XA_WINDOW || format != 32)
		return FALSE;

	wmclass_prop = XInternAtom (display, "WM_CLASS", True);
	if (wmclass_prop == None)
		return FALSE;

	gdk_error_trap_push();

	XGetWindowProperty (display, peony_window, wmclass_prop, 0, 20, False,
			    XA_STRING, &type, &format, &nitems, &after, &data);

	XSync (display, False);

	if (gdk_error_trap_pop() == BadWindow || data == NULL)
		return FALSE;

	/* See: peony_desktop_window_new(), in src/peony-desktop-window.c */
	if (nitems == 21 && after == 0 && format == 8 &&
	    !strcmp((char*) data, "desktop_window") &&
	    !strcmp((char*) data + strlen((char*) data) + 1, "Peony"))
	{
		running = TRUE;
	}
	XFree (data);

	return running;
}

static void
free_fade (UsdBackgroundManager *manager)
{
	if (manager->priv->fade != NULL) {
		g_object_unref (manager->priv->fade);
		manager->priv->fade = NULL;
	}
}

static void
free_bg_surface (UsdBackgroundManager *manager)
{
	if (manager->priv->surface != NULL) {
		cairo_surface_destroy (manager->priv->surface);
		manager->priv->surface = NULL;
	}
}

static void
free_scr_sizes (UsdBackgroundManager *manager)
{
	if (manager->priv->scr_sizes != NULL) {
		g_list_foreach (manager->priv->scr_sizes, (GFunc)g_free, NULL);
		g_list_free (manager->priv->scr_sizes);
		manager->priv->scr_sizes = NULL;
	}
}

static void
real_draw_bg (UsdBackgroundManager *manager,
	      GdkScreen		   *screen)
{
	UsdBackgroundManagerPrivate *p = manager->priv;
	GdkWindow *window = gdk_screen_get_root_window (screen);
	gint width   = gdk_screen_get_width (screen);
	gint height  = gdk_screen_get_height (screen);

	free_bg_surface (manager);
	p->surface = mate_bg_create_surface (p->bg, window, width, height, TRUE);

	if (p->do_fade)
	{
		free_fade (manager);
		p->fade = mate_bg_set_surface_as_root_with_crossfade (screen, p->surface);
		g_signal_connect_swapped (p->fade, "finished", G_CALLBACK (free_fade), manager);
	}
	else
	{
		mate_bg_set_surface_as_root (screen, p->surface);
	}
	p->scr_sizes = g_list_prepend (p->scr_sizes, g_strdup_printf ("%dx%d", width, height));
}

static void
draw_background (UsdBackgroundManager *manager,
		 gboolean              may_fade)
{
	UsdBackgroundManagerPrivate *p = manager->priv;

	if (!p->usd_can_draw || p->draw_in_progress || peony_is_drawing_bg (manager))
		return;

	ukui_settings_profile_start (NULL);

	GdkDisplay *display   = gdk_display_get_default ();
	int         n_screens = gdk_display_get_n_screens (display);
	int         scr;

	p->draw_in_progress = TRUE;
	p->do_fade = may_fade && can_fade_bg (manager);
	free_scr_sizes (manager);

	for (scr = 0; scr < n_screens; scr++)
	{
		g_debug ("Drawing background on Screen%d", scr);
		real_draw_bg (manager, gdk_display_get_screen (display, scr));
	}
	p->scr_sizes = g_list_reverse (p->scr_sizes);

	p->draw_in_progress = FALSE;
	ukui_settings_profile_end (NULL);
}

static void
on_bg_changed (MateBG               *bg,
	       UsdBackgroundManager *manager)
{
	g_debug ("Background changed");
	draw_background (manager, TRUE);
}

static void
on_bg_transitioned (MateBG               *bg,
		    UsdBackgroundManager *manager)
{
	g_debug ("Background transitioned");
	draw_background (manager, FALSE);
}

static void
on_screen_size_changed (GdkScreen            *screen,
			UsdBackgroundManager *manager)
{
	UsdBackgroundManagerPrivate *p = manager->priv;

	if (!p->usd_can_draw || p->draw_in_progress || peony_is_drawing_bg (manager))
		return;

	gint scr_num = gdk_screen_get_number (screen);
	gchar *old_size = g_list_nth_data (manager->priv->scr_sizes, scr_num);
	gchar *new_size = g_strdup_printf ("%dx%d", gdk_screen_get_width (screen),
						    gdk_screen_get_height (screen));
	if (g_strcmp0 (old_size, new_size) != 0)
	{
		g_debug ("Screen%d size changed: %s -> %s", scr_num, old_size, new_size);
		draw_background (manager, FALSE);
	} else {
		g_debug ("Screen%d size unchanged (%s). Ignoring.", scr_num, old_size);
	}
	g_free (new_size);
}

static void
disconnect_screen_signals (UsdBackgroundManager *manager)
{
	GdkDisplay *display   = gdk_display_get_default();
	int         n_screens = gdk_display_get_n_screens (display);
	int         i;

	for (i = 0; i < n_screens; i++)
	{
		g_signal_handlers_disconnect_by_func
			(gdk_display_get_screen (display, i),
			 G_CALLBACK (on_screen_size_changed), manager);
	}
}

static void
connect_screen_signals (UsdBackgroundManager *manager)
{
	GdkDisplay *display   = gdk_display_get_default();
	int         n_screens = gdk_display_get_n_screens (display);
	int         i;

	for (i = 0; i < n_screens; i++)
	{
		GdkScreen *screen = gdk_display_get_screen (display, i);

		g_signal_connect (screen, "monitors-changed",
				  G_CALLBACK (on_screen_size_changed), manager);
		g_signal_connect (screen, "size-changed",
				  G_CALLBACK (on_screen_size_changed), manager);
	}
}

static gboolean
settings_change_event_idle_cb (UsdBackgroundManager *manager)
{
	ukui_settings_profile_start ("settings_change_event_idle_cb");

	mate_bg_load_from_gsettings (manager->priv->bg,
				     manager->priv->settings);

	ukui_settings_profile_end ("settings_change_event_idle_cb");

	return FALSE;   /* remove from the list of event sources */
}

static gboolean
settings_change_event_cb (GSettings            *settings,
			  gpointer              keys,
			  gint                  n_keys,
			  UsdBackgroundManager *manager)
{
	UsdBackgroundManagerPrivate *p = manager->priv;

	/* Complements on_bg_handling_changed() */
	p->usd_can_draw = usd_can_draw_bg (manager);
	p->peony_can_draw = peony_can_draw_bg (manager);

	if (p->usd_can_draw && p->bg != NULL && !peony_is_drawing_bg (manager))
	{
		/* Defer signal processing to avoid making the dconf backend deadlock */
		g_idle_add ((GSourceFunc) settings_change_event_idle_cb, manager);
	}

	return FALSE;   /* let the event propagate further */
}

static void
setup_background (UsdBackgroundManager *manager)
{
	UsdBackgroundManagerPrivate *p = manager->priv;
	g_return_if_fail (p->bg == NULL);

	p->bg = mate_bg_new();

	p->draw_in_progress = FALSE;

	g_signal_connect(p->bg, "changed", G_CALLBACK (on_bg_changed), manager);

	g_signal_connect(p->bg, "transitioned", G_CALLBACK (on_bg_transitioned), manager);

	mate_bg_load_from_gsettings (p->bg, p->settings);

	connect_screen_signals (manager);

	g_signal_connect (p->settings, "change-event",
			  G_CALLBACK (settings_change_event_cb), manager);
}

static void
remove_background (UsdBackgroundManager *manager)
{
	UsdBackgroundManagerPrivate *p = manager->priv;

	disconnect_screen_signals (manager);

	g_signal_handlers_disconnect_by_func (p->settings, settings_change_event_cb, manager);

	if (p->settings != NULL) {
		g_object_unref (G_OBJECT (p->settings));
		p->settings = NULL;
	}

	if (p->bg != NULL) {
		g_object_unref (G_OBJECT (p->bg));
		p->bg = NULL;
	}

	free_scr_sizes (manager);
	free_bg_surface (manager);
	free_fade (manager);
}

static void
on_bg_handling_changed (GSettings            *settings,
			const char           *key,
			UsdBackgroundManager *manager)
{
	UsdBackgroundManagerPrivate *p = manager->priv;

	ukui_settings_profile_start (NULL);

	if (peony_is_drawing_bg (manager))
	{
		if (p->bg != NULL)
			remove_background (manager);
	}
	else if (p->usd_can_draw && p->bg == NULL)
	{
		setup_background (manager);
	}

	ukui_settings_profile_end (NULL);
}

static gboolean
queue_setup_background (UsdBackgroundManager *manager)
{
	manager->priv->timeout_id = 0;

	setup_background (manager);

	return FALSE;
}

static void
queue_timeout (UsdBackgroundManager *manager)
{
	if (manager->priv->timeout_id > 0)
		return;

	/* SessionRunning: now check if Peony is drawing background, and if not, set it.
	 *
	 * FIXME: We wait a few seconds after the session is up because Peony tells the
	 * session manager that its ready before it sets the background.
	 * https://bugzilla.gnome.org/show_bug.cgi?id=568588
	 */
	manager->priv->timeout_id =
		g_timeout_add_seconds (8, (GSourceFunc) queue_setup_background, manager);
}

static void
disconnect_session_manager_listener (UsdBackgroundManager* manager)
{
	if (manager->priv->proxy && manager->priv->proxy_signal_id) {
		g_signal_handler_disconnect (manager->priv->proxy,
					     manager->priv->proxy_signal_id);
		manager->priv->proxy_signal_id = 0;
	}
}

static void
on_session_manager_signal (GDBusProxy   *proxy,
			   const gchar  *sender_name,
			   const gchar  *signal_name,
			   GVariant     *parameters,
			   gpointer      user_data)
{
	UsdBackgroundManager *manager = USD_BACKGROUND_MANAGER (user_data);

	if (g_strcmp0 (signal_name, "SessionRunning") == 0) {
		queue_timeout (manager);
		disconnect_session_manager_listener (manager);
	}
}

static void
draw_bg_after_session_loads (UsdBackgroundManager *manager)
{
	GError *error = NULL;
	GDBusProxyFlags flags;

	flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
		G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;
	manager->priv->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
							      flags,
							      NULL, /* GDBusInterfaceInfo */
							      UKUI_SESSION_MANAGER_DBUS_NAME,
							      UKUI_SESSION_MANAGER_DBUS_PATH,
							      UKUI_SESSION_MANAGER_DBUS_NAME,
							      NULL, /* GCancellable */
							      &error);
	if (manager->priv->proxy == NULL) {
		g_warning ("Could not listen to session manager: %s",
			   error->message);
		g_error_free (error);
		return;
	}

	manager->priv->proxy_signal_id = g_signal_connect (manager->priv->proxy,
							   "g-signal",
							   G_CALLBACK (on_session_manager_signal),
							   manager);
}

gboolean
usd_background_manager_start (UsdBackgroundManager  *manager,
			      GError               **error)
{
	UsdBackgroundManagerPrivate *p = manager->priv;

	g_debug ("Starting background manager");
	ukui_settings_profile_start (NULL);

	p->settings = g_settings_new (MATE_BG_SCHEMA);

	p->usd_can_draw = usd_can_draw_bg (manager);
	p->peony_can_draw = peony_can_draw_bg (manager);

	g_signal_connect (p->settings, "changed::" MATE_BG_KEY_DRAW_BACKGROUND,
			  G_CALLBACK (on_bg_handling_changed), manager);
	g_signal_connect (p->settings, "changed::" MATE_BG_KEY_SHOW_DESKTOP,
			  G_CALLBACK (on_bg_handling_changed), manager);

	/* If Peony is set to draw the background, it is very likely in our session.
	 * But it might not be started yet, so peony_is_drawing_bg() would fail.
	 * In this case, we wait till the session is loaded, to avoid double-draws.
	 */
	if (p->usd_can_draw)
	{
		if (p->peony_can_draw)
		{
			draw_bg_after_session_loads (manager);
		}
		else
		{
			setup_background (manager);
		}
	}

	ukui_settings_profile_end (NULL);

	return TRUE;
}

void
usd_background_manager_stop (UsdBackgroundManager *manager)
{
	g_debug ("Stopping background manager");

	if (manager->priv->proxy)
	{
		disconnect_session_manager_listener (manager);
		g_object_unref (manager->priv->proxy);
	}

	if (manager->priv->timeout_id != 0) {
		g_source_remove (manager->priv->timeout_id);
		manager->priv->timeout_id = 0;
	}

	remove_background (manager);
}

static GObject*
usd_background_manager_constructor (GType                  type,
				    guint                  n_construct_properties,
				    GObjectConstructParam* construct_properties)
{
	UsdBackgroundManager *manager =
	   USD_BACKGROUND_MANAGER (
	      G_OBJECT_CLASS (usd_background_manager_parent_class)->constructor (
				type, n_construct_properties, construct_properties));

	return G_OBJECT(manager);
}

static void
usd_background_manager_finalize (GObject *object)
{
	g_return_if_fail (object != NULL);
	g_return_if_fail (USD_IS_BACKGROUND_MANAGER (object));

	UsdBackgroundManager *manager = USD_BACKGROUND_MANAGER (object);

	g_return_if_fail (manager->priv != NULL);

	G_OBJECT_CLASS(usd_background_manager_parent_class)->finalize(object);
}

static void
usd_background_manager_init (UsdBackgroundManager* manager)
{
	manager->priv = USD_BACKGROUND_MANAGER_GET_PRIVATE(manager);
}

static void
usd_background_manager_class_init (UsdBackgroundManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->constructor = usd_background_manager_constructor;
	object_class->finalize = usd_background_manager_finalize;

	g_type_class_add_private(klass, sizeof(UsdBackgroundManagerPrivate));
}

UsdBackgroundManager*
usd_background_manager_new (void)
{
	if (manager_object != NULL)
	{
		g_object_ref(manager_object);
	}
	else
	{
		manager_object = g_object_new(USD_TYPE_BACKGROUND_MANAGER, NULL);

		g_object_add_weak_pointer(manager_object, (gpointer*) &manager_object);
	}

	return USD_BACKGROUND_MANAGER(manager_object);
}
