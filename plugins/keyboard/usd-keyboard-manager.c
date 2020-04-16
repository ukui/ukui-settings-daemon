/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright © 2001 Ximian, Inc.
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
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

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#ifdef HAVE_X11_EXTENSIONS_XKB_H
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#endif

#include "ukui-settings-profile.h"
#include "usd-keyboard-manager.h"

#include "usd-keyboard-xkb.h"

#define USD_KEYBOARD_MANAGER_GET_PRIVATE(o) \
	(G_TYPE_INSTANCE_GET_PRIVATE((o), USD_TYPE_KEYBOARD_MANAGER, UsdKeyboardManagerPrivate))

#define USD_KEYBOARD_SCHEMA "org.ukui.peripherals-keyboard"

#define KEY_REPEAT         "repeat"
#define KEY_CLICK          "click"
#define KEY_RATE           "rate"
#define KEY_DELAY          "delay"
#define KEY_CLICK_VOLUME   "click-volume"

#define KEY_BELL_PITCH     "bell-pitch"
#define KEY_BELL_DURATION  "bell-duration"
#define KEY_BELL_MODE      "bell-mode"

#define KEY_NUMLOCK_STATE    "numlock-state"
#define KEY_NUMLOCK_REMEMBER "remember-numlock-state"


struct UsdKeyboardManagerPrivate {
	gboolean    have_xkb;
	gint        xkb_event_base;
	GSettings  *settings;
	int 	    old_state;
};

static void     usd_keyboard_manager_class_init  (UsdKeyboardManagerClass* klass);
static void     usd_keyboard_manager_init        (UsdKeyboardManager*      keyboard_manager);
static void     usd_keyboard_manager_finalize    (GObject*                 object);

G_DEFINE_TYPE (UsdKeyboardManager, usd_keyboard_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;


#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
static gboolean xfree86_set_keyboard_autorepeat_rate(int delay, int rate)
{
        gboolean res = FALSE;
        int      event_base_return;
        int      error_base_return;

        if (XF86MiscQueryExtension (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()),
                                    &event_base_return,
                                    &error_base_return) == True) {
                /* load the current settings */
                XF86MiscKbdSettings kbdsettings;
                XF86MiscGetKbdSettings (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), &kbdsettings);

                /* assign the new values */
                kbdsettings.delay = delay;
                kbdsettings.rate = rate;
                XF86MiscSetKbdSettings (GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), &kbdsettings);
                res = TRUE;
        }

        return res;
}
#endif /* HAVE_X11_EXTENSIONS_XF86MISC_H */

#ifdef HAVE_X11_EXTENSIONS_XKB_H
static gboolean xkb_set_keyboard_autorepeat_rate(int delay, int rate)
{
	int interval = (rate <= 0) ? 1000000 : 1000/rate;

	if (delay <= 0)
	{
		delay = 1;
	}

	return XkbSetAutoRepeatRate(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), XkbUseCoreKbd, delay, interval);
}
#endif

#ifdef HAVE_X11_EXTENSIONS_XKB_H

typedef enum {
        NUMLOCK_STATE_OFF = 0,
        NUMLOCK_STATE_ON = 1,
        NUMLOCK_STATE_UNKNOWN = 2
} NumLockState;

static void
numlock_xkb_init (UsdKeyboardManager *manager)
{
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        gboolean have_xkb;
        int opcode, error_base, major, minor;

        have_xkb = XkbQueryExtension (dpy,
                                      &opcode,
                                      &manager->priv->xkb_event_base,
                                      &error_base,
                                      &major,
                                      &minor)
                && XkbUseExtension (dpy, &major, &minor);

        if (have_xkb) {
                XkbSelectEventDetails (dpy,
                                       XkbUseCoreKbd,
                                       XkbStateNotifyMask,
                                       XkbModifierLockMask,
                                       XkbModifierLockMask);
        } else {
                g_warning ("XKB extension not available");
        }

        manager->priv->have_xkb = have_xkb;
}

/*
 * Function: capslock_set_xkb_state
 * urpose: Control capslock light
 */
static void
capslock_set_xkb_state(gboolean lock_state)
{
        unsigned int caps_mask;
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        caps_mask = XkbKeysymToModifiers (dpy, XK_Caps_Lock);
        XkbLockModifiers (dpy, XkbUseCoreKbd, caps_mask, lock_state ? caps_mask : 0);
        XSync (dpy, FALSE);
}

static unsigned
numlock_NumLock_modifier_mask (void)
{
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        return XkbKeysymToModifiers (dpy, XK_Num_Lock);
}

static void
numlock_set_xkb_state (NumLockState new_state)
{
        unsigned int num_mask;
        Display *dpy = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
        if (new_state != NUMLOCK_STATE_ON && new_state != NUMLOCK_STATE_OFF)
                return;
        num_mask = numlock_NumLock_modifier_mask ();
        XkbLockModifiers (dpy, XkbUseCoreKbd, num_mask, new_state ? num_mask : 0);
}

static NumLockState
numlock_get_settings_state (GSettings *settings)
{
        int          curr_state;
        curr_state = g_settings_get_enum (settings, KEY_NUMLOCK_STATE);
        return curr_state;
}

static void numlock_set_settings_state(GSettings *settings, NumLockState new_state)
{
        if (new_state != NUMLOCK_STATE_ON && new_state != NUMLOCK_STATE_OFF) {
                return;
        }
        g_settings_set_enum (settings, KEY_NUMLOCK_STATE, new_state);
}

static GdkFilterReturn
xkb_events_filter (GdkXEvent *xev_,
                   GdkEvent  *gdkev_,
                   gpointer   user_data)
{
        XEvent *xev = (XEvent *) xev_;
        XkbEvent *xkbev = (XkbEvent *) xev;
        UsdKeyboardManager *manager = (UsdKeyboardManager *) user_data;

        if (xev->type != manager->priv->xkb_event_base ||
            xkbev->any.xkb_type != XkbStateNotify)
                return GDK_FILTER_CONTINUE;

        if (xkbev->state.changed & XkbModifierLockMask) {
                unsigned num_mask = numlock_NumLock_modifier_mask ();
                unsigned locked_mods = xkbev->state.locked_mods;
                int numlock_state;

		        /* Determine if the capslock indicator is on and write Settings */
                gboolean caps;
                if(locked_mods == 2 || locked_mods == 18)
                    caps = g_settings_set_boolean (manager->priv->settings,"capslock-state",TRUE);
                else
                    caps = g_settings_set_boolean (manager->priv->settings,"capslock-state",FALSE);


                numlock_state = (num_mask & locked_mods) ? NUMLOCK_STATE_ON : NUMLOCK_STATE_OFF;

                if (numlock_state != manager->priv->old_state) {
                        g_settings_set_enum (manager->priv->settings,
                                             KEY_NUMLOCK_STATE,
                                             numlock_state);
                        manager->priv->old_state = numlock_state;
                }
        }

        return GDK_FILTER_CONTINUE;
}


static void
numlock_install_xkb_callback (UsdKeyboardManager *manager)
{
        if (!manager->priv->have_xkb)
                return;

        gdk_window_add_filter (NULL,
                               xkb_events_filter,
                               GINT_TO_POINTER (manager));
}

#endif /* HAVE_X11_EXTENSIONS_XKB_H */

static void
apply_bell (UsdKeyboardManager *manager)
{
        GSettings       *settings;
        XKeyboardControl kbdcontrol;
        gboolean         click;
        int              bell_volume;
        int              bell_pitch;
        int              bell_duration;
        char		*volume_string;
        int              click_volume;

        g_debug ("Applying the bell settings");
        settings      = manager->priv->settings;
        click         = g_settings_get_boolean  (settings, KEY_CLICK);
        click_volume  = g_settings_get_int   (settings, KEY_CLICK_VOLUME);

        bell_pitch    = g_settings_get_int   (settings, KEY_BELL_PITCH);
        bell_duration = g_settings_get_int   (settings, KEY_BELL_DURATION);

        volume_string = g_settings_get_string (settings, KEY_BELL_MODE);
        bell_volume   = (volume_string && !strcmp (volume_string, "on")) ? 50 : 0;
	g_free (volume_string);

        /* as percentage from 0..100 inclusive */
        if (click_volume < 0) {
                click_volume = 0;
        } else if (click_volume > 100) {
                click_volume = 100;
        }
        kbdcontrol.key_click_percent = click ? click_volume : 0;
        kbdcontrol.bell_percent = bell_volume;
        kbdcontrol.bell_pitch = bell_pitch;
        kbdcontrol.bell_duration = bell_duration;
        gdk_error_trap_push ();
        XChangeKeyboardControl (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                KBKeyClickPercent | KBBellPercent | KBBellPitch | KBBellDuration,
                                &kbdcontrol);

        XSync (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
        //gdk_error_trap_pop_ignored ();
}

static void
apply_numlock (UsdKeyboardManager *manager)
{
        GSettings *settings;
        gboolean rnumlock;

        g_debug ("Applying the num-lock settings");
        settings = manager->priv->settings;
        rnumlock = g_settings_get_boolean  (settings, KEY_NUMLOCK_REMEMBER);
        manager->priv->old_state = g_settings_get_enum (manager->priv->settings, KEY_NUMLOCK_STATE);

        gdk_error_trap_push ();
        if (rnumlock) {
                numlock_set_xkb_state (manager->priv->old_state);
        }

        XSync (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
        //gdk_error_trap_pop_ignored ();
}

static void
apply_repeat (UsdKeyboardManager *manager)
{
        GSettings       *settings;
        gboolean         repeat;
        guint            rate;
        guint            delay;

        g_debug ("Applying the repeat settings");
        settings      = manager->priv->settings;
        repeat        = g_settings_get_boolean  (settings, KEY_REPEAT);
        rate	      = g_settings_get_int  (settings, KEY_RATE);
        delay         = g_settings_get_int  (settings, KEY_DELAY);

        gdk_error_trap_push ();
        if (repeat) {
                gboolean rate_set = FALSE;

                XAutoRepeatOn (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
                /* Use XKB in preference */
                rate_set = xkb_set_keyboard_autorepeat_rate (delay, rate);

                if (!rate_set)
                        g_warning ("Neither XKeyboard not Xfree86's keyboard extensions are available,\n"
                                   "no way to support keyboard autorepeat rate settings");
        } else {
                XAutoRepeatOff (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()));
        }

        XSync (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), FALSE);
        //gdk_error_trap_pop_ignored ();
}

static void
apply_settings (GSettings          *settings,
                gchar              *key,
                UsdKeyboardManager *manager)
{
    /**
     * Fix by HB* system reboot but rnumlock not available;
    **/
#ifdef HAVE_X11_EXTENSIONS_XKB_H
    gboolean rnumlock;
    rnumlock = g_settings_get_boolean (settings, KEY_NUMLOCK_REMEMBER);
    
    if (rnumlock == 0 || key == NULL) {
        if (manager->priv->have_xkb && rnumlock) {

            
            /* Fix numlock and capslock indicator problem,
             * Negate is reset the status of the lamp.
             */
            numlock_set_xkb_state (!numlock_get_settings_state (settings));
            numlock_set_xkb_state (numlock_get_settings_state (settings));
            capslock_set_xkb_state(!g_settings_get_boolean(settings,"capslock-state"));
            capslock_set_xkb_state(g_settings_get_boolean(settings,"capslock-state"));
        }
    }
#endif /* HAVE_X11_EXTENSIONS_XKB_H */
	if (g_strcmp0 (key, KEY_CLICK) == 0||
            g_strcmp0 (key, KEY_CLICK_VOLUME) == 0 ||
            g_strcmp0 (key, KEY_BELL_PITCH) == 0 ||
            g_strcmp0 (key, KEY_BELL_DURATION) == 0 ||
            g_strcmp0 (key, KEY_BELL_MODE) == 0) {
                g_debug ("Bell setting '%s' changed, applying bell settings", key);
                apply_bell (manager);
        } else if (g_strcmp0 (key, KEY_NUMLOCK_REMEMBER) == 0) {
                g_debug ("Remember Num-Lock state '%s' changed, applying num-lock settings", key);
                apply_numlock (manager);
        } else if (g_strcmp0 (key, KEY_NUMLOCK_STATE) == 0) {
                g_debug ("Num-Lock state '%s' changed, will apply at next startup", key);
        } else if (g_strcmp0 (key, KEY_REPEAT) == 0 ||
                 g_strcmp0 (key, KEY_RATE) == 0 ||
                 g_strcmp0 (key, KEY_DELAY) == 0) {
                g_debug ("Key repeat setting '%s' changed, applying key repeat settings", key);
                apply_repeat (manager);
        } else {
                g_warning ("Unhandled settings change, key '%s'", key);
        }
}

void
usd_keyboard_manager_apply_settings (UsdKeyboardManager *manager)
{
        apply_settings (manager->priv->settings, NULL, manager);
}

static gboolean
start_keyboard_idle_cb (UsdKeyboardManager *manager)
{
        ukui_settings_profile_start (NULL);

        g_debug ("Starting keyboard manager");
        manager->priv->have_xkb = 0;
        manager->priv->settings = g_settings_new (USD_KEYBOARD_SCHEMA);

        /* Open the remember-numlock-state key value */
        g_settings_set_boolean (manager->priv->settings,KEY_NUMLOCK_REMEMBER,TRUE);

        /* Essential - xkb initialization should happen before */
        usd_keyboard_xkb_init (manager);
	
#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_xkb_init (manager);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        /* apply current settings before we install the callback */
        usd_keyboard_manager_apply_settings (manager);

        g_signal_connect (manager->priv->settings, "changed", G_CALLBACK (apply_settings), manager);

#ifdef HAVE_X11_EXTENSIONS_XKB_H
        numlock_install_xkb_callback (manager);
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        ukui_settings_profile_end (NULL);

        return FALSE;
}

gboolean
usd_keyboard_manager_start (UsdKeyboardManager *manager,
                            GError            **error)
{
        ukui_settings_profile_start (NULL);
        g_idle_add ((GSourceFunc) start_keyboard_idle_cb, manager);
	
        ukui_settings_profile_end (NULL);

        return TRUE;
}

void
usd_keyboard_manager_stop (UsdKeyboardManager *manager)
{
        UsdKeyboardManagerPrivate *p = manager->priv;

        g_debug ("Stopping keyboard manager");

        if (p->settings != NULL) {
                g_object_unref (p->settings);
                p->settings = NULL;
        }
#if HAVE_X11_EXTENSIONS_XKB_H
        if (p->have_xkb) {
                gdk_window_remove_filter (NULL,
                                          xkb_events_filter,
                                          GINT_TO_POINTER (manager));
        }
#endif /* HAVE_X11_EXTENSIONS_XKB_H */

        usd_keyboard_xkb_shutdown ();
}

static void
usd_keyboard_manager_class_init (UsdKeyboardManagerClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = usd_keyboard_manager_finalize;

        g_type_class_add_private (klass, sizeof (UsdKeyboardManagerPrivate));
}

static void
usd_keyboard_manager_init (UsdKeyboardManager *manager)
{
        manager->priv = USD_KEYBOARD_MANAGER_GET_PRIVATE (manager);
}

static void
usd_keyboard_manager_finalize (GObject *object)
{
        UsdKeyboardManager *keyboard_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (USD_IS_KEYBOARD_MANAGER (object));

        keyboard_manager = USD_KEYBOARD_MANAGER (object);
	
        /*
         * Fix after the logout, the small keyboard num light is on, and the input number is not reactive, 
         * and it needs to be pressed to enter the small keyboard number
        */
	    keyboard_manager->priv->old_state = 0;
        numlock_set_xkb_state(keyboard_manager->priv->old_state);
        
        /* Fix after logout the problem of CapsLock light */
        capslock_set_xkb_state(FALSE);

        g_return_if_fail (keyboard_manager->priv != NULL);

        G_OBJECT_CLASS (usd_keyboard_manager_parent_class)->finalize (object);
}

UsdKeyboardManager *
usd_keyboard_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (USD_TYPE_KEYBOARD_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return USD_KEYBOARD_MANAGER (manager_object);
}
