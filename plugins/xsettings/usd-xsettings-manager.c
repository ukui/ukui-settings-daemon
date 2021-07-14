/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 Rodrigo Moya
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
#include <time.h>

#include <X11/Xatom.h>
#include <X11/Xcursor/Xcursor.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <gio/gio.h>

#include "ukui-settings-profile.h"
#include "usd-xsettings-manager.h"
#include "xsettings-manager.h"
#include "fontconfig-monitor.h"

#define UKUI_XSETTINGS_MANAGER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), UKUI_TYPE_XSETTINGS_MANAGER, UkuiXSettingsManagerPrivate))

#define MOUSE_SCHEMA          "org.ukui.peripherals-mouse"
#define INTERFACE_SCHEMA      "org.mate.interface"
#define SOUND_SCHEMA          "org.mate.sound"

#define CURSOR_THEME_KEY      "cursor-theme"
#define CURSOR_SIZE_KEY       "cursor-size"

#define FONT_RENDER_SCHEMA    "org.ukui.font-rendering"
#define FONT_ANTIALIASING_KEY "antialiasing"
#define FONT_HINTING_KEY      "hinting"
#define FONT_RGBA_ORDER_KEY   "rgba-order"
#define FONT_DPI_KEY          "dpi"

#define XSETTINGS_PLUGIN_SCHEMA "org.ukui.SettingsDaemon.plugins.xsettings"
#define XSETTINGS_SCALING_KEY   "scaling-factor"
#define XSETTINGS_OVERRIDE_KEY  "overrides"

/* X servers sometimes lie about the screen's physical dimensions, so we cannot
 * compute an accurate DPI value.  When this happens, the user gets fonts that
 * are too huge or too tiny.  So, we see what the server returns:  if it reports
 * something outside of the range [DPI_LOW_REASONABLE_VALUE,
 * DPI_HIGH_REASONABLE_VALUE], then we assume that it is lying and we use
 * DPI_FALLBACK instead.
 *
 * See get_dpi_from_gsettings_or_server() below, and also
 * https://bugzilla.novell.com/show_bug.cgi?id=217790
 */
#define DPI_FALLBACK 96
#define DPI_LOW_REASONABLE_VALUE 50
#define DPI_HIGH_REASONABLE_VALUE 500

/* The minimum resolution at which we turn on a window-scale of 2 */
#define HIDPI_LIMIT (DPI_FALLBACK * 2)

/* The minimum screen height at which we turn on a window-scale of 2;
 * below this there just isn't enough vertical real estate for GNOME
 * apps to work, and it's better to just be tiny */
#define HIDPI_MIN_HEIGHT 1500

typedef struct _TranslationEntry TranslationEntry;
typedef void (* TranslationFunc) (UkuiXSettingsManager  *manager,
                                  TranslationEntry      *trans,
                                  GVariant              *value);

struct _TranslationEntry {
        const char     *gsettings_schema;
        const char     *gsettings_key;
        const char     *xsetting_name;

        TranslationFunc translate;
};

struct UkuiXSettingsManagerPrivate
{
        XSettingsManager **managers;
        GHashTable        *gsettings;
        GSettings         *gsettings_font;
        GSettings         *plugin_settings;
        fontconfig_monitor_handle_t *fontconfig_handle;
};

#define USD_XSETTINGS_ERROR usd_xsettings_error_quark ()

enum {
        USD_XSETTINGS_ERROR_INIT
};

static void     ukui_xsettings_manager_class_init  (UkuiXSettingsManagerClass *klass);
static void     ukui_xsettings_manager_init        (UkuiXSettingsManager      *xsettings_manager);
static void     ukui_xsettings_manager_finalize    (GObject                  *object);

G_DEFINE_TYPE (UkuiXSettingsManager, ukui_xsettings_manager, G_TYPE_OBJECT)

static gpointer manager_object = NULL;

static GQuark
usd_xsettings_error_quark (void)
{
        return g_quark_from_static_string ("usd-xsettings-error-quark");
}

static void
translate_bool_int (UkuiXSettingsManager  *manager,
                    TranslationEntry      *trans,
                    GVariant              *value)
{
        int i;

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], trans->xsetting_name,
                                           g_variant_get_boolean (value));
        }
}

static void
translate_int_int (UkuiXSettingsManager  *manager,
                   TranslationEntry      *trans,
                   GVariant              *value)
{
        int i;

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], trans->xsetting_name,
                                           g_variant_get_int32 (value));
        }
}

static void
translate_string_string (UkuiXSettingsManager  *manager,
                         TranslationEntry      *trans,
                         GVariant              *value)
{
        int i;

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_string (manager->priv->managers [i],
                                              trans->xsetting_name,
                                              g_variant_get_string (value, NULL));
        }
}

static void
translate_string_string_toolbar (UkuiXSettingsManager  *manager,
                                 TranslationEntry      *trans,
                                 GVariant              *value)
{
        int         i;
        const char *tmp;

        /* This is kind of a workaround since GNOME expects the key value to be
         * "both_horiz" and gtk+ wants the XSetting to be "both-horiz".
         */
        tmp = g_variant_get_string (value, NULL);
        if (tmp && strcmp (tmp, "both_horiz") == 0) {
                tmp = "both-horiz";
        }

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_string (manager->priv->managers [i],
                                              trans->xsetting_name,
                                              tmp);
        }
}

/* Func: translate_string_string_window_buttons
 * Set window button layout 
 * 设置窗口按钮布局   
 */
static void
translate_string_string_window_buttons (UkuiXSettingsManager *manager,
                                        TranslationEntry      *trans,
                                        GVariant              *value)
{
    int         i;
    const char *tmp;
    gchar *ptr, *final_str;

    /* This is kind of a workaround. "menu" is useless in metacity titlebars
     * it duplicates the same features as the right-click menu.
     * In CSD windows on the hand it is required to show unique featues.
     */
    tmp = g_variant_get_string (value, NULL);

    /* Check if menu is in the setting string already */
    ptr = g_strstr_len (tmp, -1, "menu");

    if (!ptr) {
        /* If it wasn't there already, we add it... */

        /* Simple cases, :* - all items on right, just prepend menu on left side*/
        if (g_str_has_prefix (tmp, ":")) {
            final_str = g_strdup_printf ("menu%s", tmp);
       }
       else
        /* All items on left... * (no :), append menu - we want actual window
           controls on the outside */
        if (!g_strstr_len (tmp, -1, ":")) {
            final_str = g_strdup_printf ("%s,menu", tmp);
        }
        else {
            /* Items on both sides, split it, append menu to the lefthand, and re-
             * construct the string with the : separator */
            gchar **split = g_strsplit (tmp, ":", 2);
            final_str = g_strdup_printf ("%s,menu:%s", split[0], split[1]);
            
            g_strfreev (split);
        }
   } else {
       /* If menu was already included, just copy the original string */
        final_str = g_strdup (tmp);
    }

    for (i = 0; manager->priv->managers [i]; i++) {
            xsettings_manager_set_string (manager->priv->managers [i],
                                          trans->xsetting_name,
                                          final_str);
    }
    g_free (final_str);
}

static TranslationEntry translations [] = {
        { MOUSE_SCHEMA,     "double-click",           "Net/DoubleClickTime",           translate_int_int },
        { MOUSE_SCHEMA,     "drag-threshold",         "Net/DndDragThreshold",          translate_int_int },
        { MOUSE_SCHEMA,     "cursor-theme",           "Gtk/CursorThemeName",           translate_string_string },
        { MOUSE_SCHEMA,     "cursor-size",            "Gtk/CursorThemeSize",           translate_int_int },

        { INTERFACE_SCHEMA, "font-name",              "Gtk/FontName",                  translate_string_string },
        { INTERFACE_SCHEMA, "gtk-key-theme",          "Gtk/KeyThemeName",              translate_string_string },
        { INTERFACE_SCHEMA, "toolbar-style",          "Gtk/ToolbarStyle",              translate_string_string_toolbar },
        { INTERFACE_SCHEMA, "toolbar-icons-size",     "Gtk/ToolbarIconSize",           translate_string_string },
        { INTERFACE_SCHEMA, "cursor-blink",           "Net/CursorBlink",               translate_bool_int },
        { INTERFACE_SCHEMA, "cursor-blink-time",      "Net/CursorBlinkTime",           translate_int_int },
        { INTERFACE_SCHEMA, "gtk-theme",              "Net/ThemeName",                 translate_string_string },
        { INTERFACE_SCHEMA, "gtk-color-scheme",       "Gtk/ColorScheme",               translate_string_string },
        { INTERFACE_SCHEMA, "gtk-im-preedit-style",   "Gtk/IMPreeditStyle",            translate_string_string },
        { INTERFACE_SCHEMA, "gtk-im-status-style",    "Gtk/IMStatusStyle",             translate_string_string },
        { INTERFACE_SCHEMA, "gtk-im-module",          "Gtk/IMModule",                  translate_string_string },
        { INTERFACE_SCHEMA, "icon-theme",             "Net/IconThemeName",             translate_string_string },
        { INTERFACE_SCHEMA, "file-chooser-backend",   "Gtk/FileChooserBackend",        translate_string_string },
        
        { INTERFACE_SCHEMA, "gtk-decoration-layout",  "Gtk/DecorationLayout",          translate_string_string_window_buttons },

        { INTERFACE_SCHEMA, "gtk-shell-shows-app-menu","Gtk/ShellShowsAppMenu",        translate_bool_int },
        { INTERFACE_SCHEMA, "gtk-shell-shows-menubar","Gtk/ShellShowsMenubar",         translate_bool_int },
        { INTERFACE_SCHEMA, "menus-have-icons",       "Gtk/MenuImages",                translate_bool_int },
        { INTERFACE_SCHEMA, "buttons-have-icons",     "Gtk/ButtonImages",              translate_bool_int },
        { INTERFACE_SCHEMA, "menubar-accel",          "Gtk/MenuBarAccel",              translate_string_string },
        { INTERFACE_SCHEMA, "show-input-method-menu", "Gtk/ShowInputMethodMenu",       translate_bool_int },
        { INTERFACE_SCHEMA, "show-unicode-menu",      "Gtk/ShowUnicodeMenu",           translate_bool_int },
        { INTERFACE_SCHEMA, "automatic-mnemonics",    "Gtk/AutoMnemonics",             translate_bool_int },
        { INTERFACE_SCHEMA, "gtk-enable-animations",  "Gtk/EnableAnimations",          translate_bool_int },
        { INTERFACE_SCHEMA, "gtk-dialogs-use-header", "Gtk/DialogsUseHeader",          translate_bool_int },

        { SOUND_SCHEMA, "theme-name",                 "Net/SoundThemeName",            translate_string_string },
        { SOUND_SCHEMA, "event-sounds",               "Net/EnableEventSounds" ,        translate_bool_int },
        { SOUND_SCHEMA, "input-feedback-sounds",      "Net/EnableInputFeedbackSounds", translate_bool_int }
};

/* Auto-detect the most appropriate scale factor for the primary monitor.
 * A lot of this code is copied and adapted from Linux Mint/Cinnamon.
 * 如果设置缩放为0，通过获取物理显示器的分辨率大小进行设置合适的DPI缩放
 */
static int
get_window_scale_auto ()
{
        GdkDisplay   *display;
        GdkMonitor   *monitor;
        GdkRectangle  rect;
        int width_mm, height_mm;
        int monitor_scale, window_scale;

        display = gdk_display_get_default ();
        monitor = gdk_display_get_primary_monitor (display);

        /* Use current value as the default */
        window_scale = 1;

        gdk_monitor_get_geometry (monitor, &rect);
        width_mm = gdk_monitor_get_width_mm (monitor);
        height_mm = gdk_monitor_get_height_mm (monitor);
        monitor_scale = gdk_monitor_get_scale_factor (monitor);

        if (rect.height * monitor_scale < HIDPI_MIN_HEIGHT)
                return 1;

        /* Some monitors/TV encode the aspect ratio (16/9 or 16/10) instead of the physical size */
        if ((width_mm == 160 && height_mm == 90) ||
            (width_mm == 160 && height_mm == 100) ||
            (width_mm == 16 && height_mm == 9) ||
            (width_mm == 16 && height_mm == 10))
                return 1;

        if (width_mm > 0 && height_mm > 0) {
                double dpi_x, dpi_y;

                dpi_x = (double)rect.width * monitor_scale / (width_mm / 25.4);
                dpi_y = (double)rect.height * monitor_scale / (height_mm / 25.4);
                /* We don't completely trust these values so both must be high, and never pick
                 * higher ratio than 2 automatically */
                if (dpi_x > HIDPI_LIMIT && dpi_y > HIDPI_LIMIT)
                        window_scale = 2;
        }

        return window_scale;
}

/* Get the key value to set the zoom
 * 获取要设置缩放的键值
 */
static double
get_window_scale (UkuiXSettingsManager *manager)
{
        GSettings   *gsettings;
        double         scale;

        /* Get scale factor from gsettings */
        gsettings = g_hash_table_lookup (manager->priv->gsettings, XSETTINGS_PLUGIN_SCHEMA);
        scale = g_settings_get_double (gsettings, XSETTINGS_SCALING_KEY);

        /* Auto-detect */
        if (scale == 0)
                scale = get_window_scale_auto ();

        return scale;
}

static double
dpi_from_pixels_and_mm (int pixels,
                        int mm)
{
        double dpi;

        if (mm >= 1)
                dpi = pixels / (mm / 25.4);
        else
                dpi = 0;

        return dpi;
}

static double
get_dpi_from_x_server (void)
{
        GdkScreen *screen;
        double     dpi;

        screen = gdk_screen_get_default ();
        if (screen != NULL) {
                double width_dpi, height_dpi;
                Screen *xscreen = gdk_x11_screen_get_xscreen (screen);

                width_dpi = dpi_from_pixels_and_mm (WidthOfScreen (xscreen), WidthMMOfScreen (xscreen)); //gdk_screen_get_width (screen), gdk_screen_get_width_mm (screen));
                height_dpi = dpi_from_pixels_and_mm (HeightOfScreen (xscreen), HeightMMOfScreen (xscreen)); //gdk_screen_get_height (screen), gdk_screen_get_height_mm (screen));

                if (width_dpi < DPI_LOW_REASONABLE_VALUE || width_dpi > DPI_HIGH_REASONABLE_VALUE
                    || height_dpi < DPI_LOW_REASONABLE_VALUE || height_dpi > DPI_HIGH_REASONABLE_VALUE) {
                        dpi = DPI_FALLBACK;
                } else {
                        dpi = (width_dpi + height_dpi) / 2.0;
                }
        } else {
                /* Huh!?  No screen? */

                dpi = DPI_FALLBACK;
        }

        return dpi;
}

static double
get_dpi_from_gsettings_or_x_server (GSettings *gsettings)
{
        double dpi;

        dpi = g_settings_get_double (gsettings, FONT_DPI_KEY);

        /* If the user has ever set the DPI preference in GSettings, we use that.
         * Otherwise, we see if the X server reports a reasonable DPI value:  some X
         * servers report completely bogus values, and the user gets huge or tiny
         * fonts which are unusable.
         */

        if (dpi == 0) {
                dpi = DPI_FALLBACK;
                //dpi = get_dpi_from_x_server ();
        }

        return dpi;
}

typedef struct
{
        gboolean    antialias;
        gboolean    hinting;

        int         scaled_dpi;
        double      window_scale;
        int         dpi;

        char       *cursor_theme;
        int         cursor_size;
        const char *rgba;
        const char *hintstyle;
} UkuiXftSettings;

static const char *rgba_types[] = { "rgb", "bgr", "vbgr", "vrgb" };

/* Read GSettings values and determine the appropriate Xft settings based on them
 * This probably could be done a bit more cleanly with g_settings_get_enum
 */
static void
xft_settings_get (UkuiXSettingsManager *manager,
                  UkuiXftSettings *settings)
{
        GSettings *mouse_gsettings;
        char      *antialiasing;
        char      *hinting;
        char      *rgba_order;
        double     dpi;
        double     scale;

        mouse_gsettings = g_hash_table_lookup (manager->priv->gsettings, MOUSE_SCHEMA);

        antialiasing = g_settings_get_string (manager->priv->gsettings_font, FONT_ANTIALIASING_KEY);
        hinting = g_settings_get_string (manager->priv->gsettings_font, FONT_HINTING_KEY);
        rgba_order = g_settings_get_string (manager->priv->gsettings_font, FONT_RGBA_ORDER_KEY);
        dpi = get_dpi_from_gsettings_or_x_server (manager->priv->gsettings_font);
        scale = get_window_scale (manager);

        /*
        if (scale >= 0 && scale <= 1.5) {
            settings->window_scale = 1;
        } else if (scale >= 1.75 && scale <= 2.5) {
            settings->window_scale = 2;
        } else if (scale >= 2.75) {
            settings->window_scale = 3;
        }
        */
        settings->window_scale = scale;

        settings->antialias = TRUE;
        settings->hinting = TRUE;
        settings->hintstyle = "hintslight";
        settings->dpi = dpi * 1024; /* Xft wants 1/1024ths of an inch */
        settings->scaled_dpi = dpi * scale * 1024;

        settings->cursor_theme = g_settings_get_string (mouse_gsettings, CURSOR_THEME_KEY);
        settings->cursor_size = g_settings_get_int (mouse_gsettings, CURSOR_SIZE_KEY);
        settings->rgba = "rgb";

        if (rgba_order) {
                int i;
                gboolean found = FALSE;

                for (i = 0; i < G_N_ELEMENTS (rgba_types) && !found; i++) {
                        if (strcmp (rgba_order, rgba_types[i]) == 0) {
                                settings->rgba = rgba_types[i];
                                found = TRUE;
                        }
                }

                if (!found) {
                        g_warning ("Invalid value for " FONT_RGBA_ORDER_KEY ": '%s'",
                                   rgba_order);
                }
        }

        if (hinting) {
                if (strcmp (hinting, "none") == 0) {
                        settings->hinting = 0;
                        settings->hintstyle = "hintnone";
                } else if (strcmp (hinting, "slight") == 0) {
                        settings->hinting = 1;
                        settings->hintstyle = "hintslight";
                } else if (strcmp (hinting, "medium") == 0) {
                        settings->hinting = 1;
                        settings->hintstyle = "hintmedium";
                } else if (strcmp (hinting, "full") == 0) {
                        settings->hinting = 1;
                        settings->hintstyle = "hintfull";
                } else {
                        g_warning ("Invalid value for " FONT_HINTING_KEY ": '%s'",
                                   hinting);
                }
        }

        if (antialiasing) {
                gboolean use_rgba = FALSE;

                if (strcmp (antialiasing, "none") == 0) {
                        settings->antialias = 0;
                } else if (strcmp (antialiasing, "grayscale") == 0) {
                        settings->antialias = 1;
                } else if (strcmp (antialiasing, "rgba") == 0) {
                        settings->antialias = 1;
                        use_rgba = TRUE;
                } else {
                        g_warning ("Invalid value for " FONT_ANTIALIASING_KEY " : '%s'",
                                   antialiasing);
                }

                if (!use_rgba) {
                        settings->rgba = "none";
                }
        }

        g_free (rgba_order);
        g_free (hinting);
        g_free (antialiasing);
}

static void
xft_settings_set_xsettings (UkuiXSettingsManager *manager,
                            UkuiXftSettings      *settings)
{
        int i;
        ukui_settings_profile_start (NULL);
        double scale = get_window_scale(manager);
        if (scale >= 2.0)
              scale = scale - 1.0;
        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], "Xft/Antialias", settings->antialias);
                xsettings_manager_set_int (manager->priv->managers [i], "Xft/Hinting", settings->hinting);
                xsettings_manager_set_string (manager->priv->managers [i], "Xft/HintStyle", settings->hintstyle);
                
                xsettings_manager_set_int (manager->priv->managers [i], "Gdk/WindowScalingFactor", settings->window_scale);
                xsettings_manager_set_int (manager->priv->managers [i], "Gdk/UnscaledDPI", (double)settings->dpi * scale);
                xsettings_manager_set_int (manager->priv->managers [i], "Xft/DPI", settings->scaled_dpi);
                
                xsettings_manager_set_string (manager->priv->managers [i], "Xft/RGBA", settings->rgba);
                xsettings_manager_set_string (manager->priv->managers [i], "Xft/lcdfilter",
                                              g_str_equal (settings->rgba, "rgb") ? "lcddefault" : "none");
                xsettings_manager_set_int (manager->priv->managers [i], "Gtk/CursorThemeSize", settings->cursor_size);
                xsettings_manager_set_string (manager->priv->managers [i], "Gtk/CursorThemeName", settings->cursor_theme);
                GdkCursor *cursor = gdk_cursor_new_for_display(gdk_display_get_default(),
                                                               GDK_LEFT_PTR);
                gdk_window_set_cursor(gdk_get_default_root_window(), cursor);
                g_object_unref(G_OBJECT(cursor));
        }
        ukui_settings_profile_end (NULL);
}

static void
update_property (GString *props, const gchar* key, const gchar* value)
{
        gchar* needle;
        size_t needle_len;
        gchar* found = NULL;

        /* update an existing property */
        needle = g_strconcat (key, ":", NULL);
        needle_len = strlen (needle);
        if (g_str_has_prefix (props->str, needle))
                found = props->str;
        else
                found = strstr (props->str, needle);

        if (found) {
                size_t value_index;
                gchar* end;

                end = strchr (found, '\n');
                value_index = (found - props->str) + needle_len + 1;
                g_string_erase (props, value_index, end ? (end - found - needle_len) : -1);
                g_string_insert (props, value_index, "\n");
                g_string_insert (props, value_index, value);
        } else {
                g_string_append_printf (props, "%s:\t%s\n", key, value);
        }

        g_free (needle);
}

static void
xft_settings_set_xresources (UkuiXftSettings *settings)
{
        GString    *add_string;
        char        dpibuf[G_ASCII_DTOSTR_BUF_SIZE];
        Display    *dpy;

        ukui_settings_profile_start (NULL);

        /* get existing properties */
        dpy = XOpenDisplay (NULL);
        g_return_if_fail (dpy != NULL);
        add_string = g_string_new (XResourceManagerString (dpy));

        g_debug("xft_settings_set_xresources: orig res '%s'", add_string->str);

        // add for apply Qt APP when control-center set cursor
	char tmpCursorTheme[255] = {'\0'};
	int tmpCursorSize = 0;
	if (strlen (settings->cursor_theme) > 0) {
		strncpy(tmpCursorTheme, settings->cursor_theme, 255);
	} else {
		// unset, use default
		strncpy(tmpCursorTheme, "DMZ-White", 255);
	}
	if (settings->cursor_size > 0) {
		tmpCursorSize = settings->cursor_size;
	} else {
		tmpCursorSize = XcursorGetDefaultSize(dpy);
	}
	// end add by liutong

    /* 添加DPI缩放至文件
     * Add DPI zoom to file */
    /*
    gchar    *XreFilePath;
    gchar    *date;
    gboolean  res;
    XreFilePath = g_build_filename (g_get_home_dir (), ".Xresources", NULL);
    date = g_malloc (128);
    g_sprintf(date,"Xft.dpi:%d\nXcursor.size:%d\nXcursor.theme:%s\n",
                    settings->scaled_dpi/1024, settings->cursor_size, tmpCursorTheme);
    res = g_file_set_contents(XreFilePath,date,strlen(date),NULL);
    if(!res)
        g_debug("Xresources File write failed ");
    */
    /* end add by Shang Xiaoyang */
        update_property (add_string, "Xft.dpi",
                                g_ascii_dtostr (dpibuf, sizeof (dpibuf), (double) settings->scaled_dpi / 1024.0));
        update_property (add_string, "Xft.antialias",
                                settings->antialias ? "1" : "0");
        update_property (add_string, "Xft.hinting",
                                settings->hinting ? "1" : "0");
        update_property (add_string, "Xft.hintstyle",
                                settings->hintstyle);
        update_property (add_string, "Xft.rgba",
                                settings->rgba);
        update_property (add_string, "Xft.lcdfilter",
                         g_str_equal (settings->rgba, "rgb") ? "lcddefault" : "none");
        update_property (add_string, "Xcursor.theme",
                                settings->cursor_theme);
        update_property (add_string, "Xcursor.size",
                                g_ascii_dtostr (dpibuf, sizeof (dpibuf), (double) settings->cursor_size));

        g_debug("xft_settings_set_xresources: new res '%s'", add_string->str);

        /* Set the new X property */
        XChangeProperty(dpy, RootWindow (dpy, 0),
                XA_RESOURCE_MANAGER, XA_STRING, 8, PropModeReplace, (unsigned char *) add_string->str, add_string->len);

	// begin add:for qt adjust cursor size&theme. add by liutong
	const char *CursorsNames[] = {
                "X_cursor"       , "arrow"             , "bottom_side"        , "bottom_tee"  ,
                "bd_double_arrow", "bottom_left_corner", "bottom_right_corner", "color-picker",
                "cross"         , "cross_reverse"    , "copy"            , "circle"     ,
                "crossed_circle", "dnd-ask"          , "dnd-copy"        , "dnd-link"   ,
                "dnd-move"      , "dnd-none"         , "dot_box_mask"    , "fd_double_arrow",
                "crosshair"     , "diamond_cross"    , "dotbox"          , "draped_box" ,
                "double_arrow"  , "draft_large"      , "draft_small"     , "fleur"      ,
                "grabbing"      , "help"             , "hand",  "hand1"  , "hand2"      ,
                "icon"          , "left_ptr"         , "left_side"       , "left_tee"   ,
                "left_ptr_watch", "ll_angle"         , "lr_angle"        , "link"       ,
                "move"          , "pencil"           , "pirate"          , "plus"       ,
                "question_arrow", "right_ptr"        , "right_side"      , "right_tee"  ,
                "sb_down_arrow" , "sb_h_double_arrow", "sb_left_arrow"   , "sb_right_arrow" ,
                "sb_up_arrow"   , "sb_v_double_arrow", "target"          , "tcross"     ,
                "top_left_arrow", "top_left_corner"  , "top_right_corner", "top_side"   ,
                "top_tee"       , "ul_angle"         , "ur_angle"        , "watch"     ,
                "xterm"         , "h_double_arrow"   , "v_double_arrow"  , "left_ptr_help",
                NULL};

	if (strlen (tmpCursorTheme) > 0 ) {
	    int len = sizeof(CursorsNames)/sizeof(*CursorsNames);
	    for (int i = 0; i < len-1; i++) {
            XcursorImages *images = XcursorLibraryLoadImages(CursorsNames[i], tmpCursorTheme, tmpCursorSize);
	        if (!images) {
                g_debug("xcursorlibrary load images :null image, theme name=%s", tmpCursorTheme);
		        continue;
	        }
	        Cursor handle = XcursorImagesLoadCursor(dpy, images);
            int event_base, error_base;
            if (XFixesQueryExtension(dpy, &event_base, &error_base))
            {
                int major, minor;
                XFixesQueryVersion(dpy, &major, &minor);
                if (major >= 2) {
                    g_debug("set CursorNmae=%s", CursorsNames[i]);
                    XFixesSetCursorName(dpy, handle, CursorsNames[i]);
                }
            }
            XFixesChangeCursorByName(dpy, handle, CursorsNames[i]);
            XcursorImagesDestroy(images);
        }
	}
	// end add

        XCloseDisplay (dpy);

        g_string_free (add_string, TRUE);

        ukui_settings_profile_end (NULL);
}

/* We mirror the Xft properties both through XSETTINGS and through
 * X resources
 */
static void
update_xft_settings (UkuiXSettingsManager *manager)
{
        UkuiXftSettings settings;

        ukui_settings_profile_start (NULL);

        xft_settings_get (manager, &settings);
        xft_settings_set_xsettings (manager, &settings);
        xft_settings_set_xresources (&settings);

        ukui_settings_profile_end (NULL);
}

static void
xft_callback (GSettings            *gsettings,
              const gchar          *key,
              UkuiXSettingsManager *manager)
{
        int i;

        update_xft_settings (manager);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }
}

/* 屏幕分辨率变化回调函数 */
static void
size_changed_callback (GdkScreen *screen, UkuiXSettingsManager *manager)
{
    int i;

    update_xft_settings (manager);

    for (i = 0; manager->priv->managers [i]; i++) {
            xsettings_manager_notify (manager->priv->managers [i]);
    }
}

/* 键值更改回调函数 */
static void
override_callback (GSettings             *settings,
                   const gchar           *key,
                   UkuiXSettingsManager *manager)
{
        GVariant *value;
        int i;

        value = g_settings_get_value (settings, XSETTINGS_OVERRIDE_KEY);

        for (i = 0; manager->priv->managers[i]; i++) {
                xsettings_manager_set_overrides (manager->priv->managers[i], value);
        }

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }
        g_variant_unref (value);
}

/* 监听gsettings回调 */
static void
plugin_callback (GSettings             *settings,
                 const char            *key,
                 UkuiXSettingsManager  *manager)
{

    if (g_str_equal (key, XSETTINGS_OVERRIDE_KEY)) {
        override_callback (settings, key, manager);
    }
}

static void
fontconfig_callback (fontconfig_monitor_handle_t *handle,
                     UkuiXSettingsManager       *manager)
{
        int i;
        int timestamp = time (NULL);

        ukui_settings_profile_start (NULL);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_int (manager->priv->managers [i], "Fontconfig/Timestamp", timestamp);
                xsettings_manager_notify (manager->priv->managers [i]);
        }
        ukui_settings_profile_end (NULL);
}

static gboolean
start_fontconfig_monitor_idle_cb (UkuiXSettingsManager *manager)
{
        ukui_settings_profile_start (NULL);

        manager->priv->fontconfig_handle = fontconfig_monitor_start ((GFunc) fontconfig_callback, manager);

        ukui_settings_profile_end (NULL);

        return FALSE;
}

static void
start_fontconfig_monitor (UkuiXSettingsManager  *manager)
{
        ukui_settings_profile_start (NULL);

        fontconfig_cache_init ();

        g_idle_add ((GSourceFunc) start_fontconfig_monitor_idle_cb, manager);

        ukui_settings_profile_end (NULL);
}

static void
stop_fontconfig_monitor (UkuiXSettingsManager  *manager)
{
        if (manager->priv->fontconfig_handle) {
                fontconfig_monitor_stop (manager->priv->fontconfig_handle);
                manager->priv->fontconfig_handle = NULL;
        }
}

static void
process_value (UkuiXSettingsManager *manager,
               TranslationEntry     *trans,
               GVariant             *value)
{
        (* trans->translate) (manager, trans, value);
}

static TranslationEntry *
find_translation_entry (GSettings *gsettings, const char *key)
{
        guint i;
        char *schema;

        g_object_get (gsettings, "schema", &schema, NULL);

        for (i = 0; i < G_N_ELEMENTS (translations); i++) {
                if (g_str_equal (schema, translations[i].gsettings_schema) &&
                    g_str_equal (key, translations[i].gsettings_key)) {
                            g_free (schema);
                        return &translations[i];
                }
        }

        g_free (schema);

        return NULL;
}

static void
xsettings_callback (GSettings             *gsettings,
                    const char            *key,
                    UkuiXSettingsManager  *manager)
{
        TranslationEntry *trans;
        int               i;
        GVariant         *value;

        if (g_str_equal (key, XSETTINGS_SCALING_KEY )||
            g_str_equal (key, CURSOR_THEME_KEY )||
            g_str_equal (key, CURSOR_SIZE_KEY  )){
                xft_callback (NULL, key, manager);
                return;
	    }

        trans = find_translation_entry (gsettings, key);
        if (trans == NULL) {
                return;
        }

        value = g_settings_get_value (gsettings, key);

        process_value (manager, trans, value);

        g_variant_unref (value);

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_set_string (manager->priv->managers [i],
                                              "Net/FallbackIconTheme",
                                              "ukui");
        }

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }
}

static void
terminate_cb (void *data)
{
        gboolean *terminated = data;

        if (*terminated) {
                return;
        }

        *terminated = TRUE;

        gtk_main_quit ();
}

static gboolean
setup_xsettings_managers (UkuiXSettingsManager *manager)
{
        GdkDisplay *display;
        int         i;
        int         n_screens;
        gboolean    res;
        gboolean    terminated;

        display = gdk_display_get_default ();
        n_screens = gdk_display_get_n_screens (display);

        res = xsettings_manager_check_running (gdk_x11_display_get_xdisplay (display),
                                               gdk_screen_get_number (gdk_screen_get_default ()));
        if (res) {
                g_warning ("You can only run one xsettings manager at a time; exiting");
                return FALSE;
        }

        manager->priv->managers = g_new0 (XSettingsManager *, n_screens + 1);

        terminated = FALSE;
        for (i = 0; i < n_screens; i++) {
                GdkScreen *screen;

                screen = gdk_display_get_screen (display, i);

                manager->priv->managers [i] = xsettings_manager_new (gdk_x11_display_get_xdisplay (display),
                                                                     gdk_screen_get_number (screen),
                                                                     terminate_cb,
                                                                     &terminated);
                if (! manager->priv->managers [i]) {
                        g_warning ("Could not create xsettings manager for screen %d!", i);
                        return FALSE;
                }
                g_signal_connect (screen, "size-changed", G_CALLBACK (size_changed_callback), manager);
        }

        return TRUE;
}
void send_session_dbus()
{
    GDBusConnection *con = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);
    g_dbus_connection_call_sync (con,
                                 "org.gnome.SessionManager",
                                 "/org/gnome/SessionManager",
                                 "org.gnome.SessionManager",
                                 "startupfinished",
                                 g_variant_new ("(ss)",
                                                "ukui-settings-daemon",
                                                "startupfinished"),
                                 NULL,
                                 G_DBUS_CALL_FLAGS_NONE,
                                 -1,
                                 NULL,
                                 NULL);
    g_object_unref (con);
}

static void
update_scale_settings (UkuiXSettingsManager *manager)
{
        GdkScreen   *screen;
        GdkRectangle dest_left, dest_right;
        int width , height, monitor, monitor_left, monitor_right, screen_num;
        GSettings   *gsettings;
        double       scale;
        screen = gdk_screen_get_default();
        width = gdk_screen_get_width(screen);
        height = gdk_screen_get_height(screen);
        screen_num = gdk_screen_get_n_monitors(screen);

        monitor_left = gdk_screen_get_monitor_at_point(screen,0,0);
        gdk_screen_get_monitor_geometry(screen, monitor_left, &dest_left);

        if (screen_num > 1){
            monitor_right = gdk_screen_get_monitor_at_point(screen,width,0);
            gdk_screen_get_monitor_geometry(screen, monitor_right, &dest_right);
        }

        gsettings = g_settings_new (XSETTINGS_PLUGIN_SCHEMA);
        scale = g_settings_get_double (gsettings, XSETTINGS_SCALING_KEY);
        if(scale > 1.25){
            gboolean state = FALSE;
            if (screen_num == 1){
                if (dest_left.width < 1920 && dest_left.height < 1080){
                    state = TRUE;
                } else if (dest_left.width == 1920 && dest_left.height == 1080 && scale > 1.5){
                    state = TRUE;
                }
            } else if (screen_num > 1){
                if ((dest_left.width < 1920 && dest_left.height < 1080) ||
                    (dest_right.width < 1920 && dest_right.height < 1080)) {
                        state = TRUE;
                } else if (((dest_left.width == 1920 && dest_left.height == 1080)    ||
                            (dest_right.width == 1920 && dest_right.height == 1080)) &&
                             scale > 1.5) {
                        state = TRUE;
                }
            } else {
                state = FALSE;
            }
            if (state) {
                GSettings   *mGsettings;
                mGsettings = g_settings_new (MOUSE_SCHEMA);
                g_settings_set_int (mGsettings, CURSOR_SIZE_KEY, 24);
                g_settings_set_double (gsettings, XSETTINGS_SCALING_KEY, 1.0);
                g_object_unref (mGsettings);
            }
        }
        g_object_unref (gsettings);
}

gboolean
ukui_xsettings_manager_start (UkuiXSettingsManager *manager,
                               GError               **error)
{
        guint        i;
        GList       *list, *l;
        GVariant    *overrides;

        g_debug ("Starting xsettings manager");
        ukui_settings_profile_start (NULL);

        update_scale_settings (manager);

        if (!setup_xsettings_managers (manager)) {
                g_set_error (error, USD_XSETTINGS_ERROR,
                             USD_XSETTINGS_ERROR_INIT,
                             "Could not initialize xsettings manager.");
                return FALSE;
        }

        manager->priv->gsettings = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                         NULL, (GDestroyNotify) g_object_unref);

        g_hash_table_insert (manager->priv->gsettings,
                             MOUSE_SCHEMA, g_settings_new (MOUSE_SCHEMA));
        g_hash_table_insert (manager->priv->gsettings,
                             INTERFACE_SCHEMA, g_settings_new (INTERFACE_SCHEMA));
        g_hash_table_insert (manager->priv->gsettings,
                             SOUND_SCHEMA, g_settings_new (SOUND_SCHEMA));
        g_hash_table_insert (manager->priv->gsettings,
                             XSETTINGS_PLUGIN_SCHEMA, g_settings_new (XSETTINGS_PLUGIN_SCHEMA));

        list = g_hash_table_get_values (manager->priv->gsettings);
        for (l = list; l != NULL; l = l->next) {
                g_signal_connect_object (G_OBJECT (l->data), "changed",
                			 G_CALLBACK (xsettings_callback), manager, 0);
        }
        g_list_free (list);

        for (i = 0; i < G_N_ELEMENTS (translations); i++) {
                GVariant  *val;
                GSettings *gsettings;

                gsettings = g_hash_table_lookup (manager->priv->gsettings,
                                                translations[i].gsettings_schema);

		        if (gsettings == NULL) {
			        g_warning ("Schemas '%s' has not been setup", translations[i].gsettings_schema);
			        continue;
		        }

                val = g_settings_get_value (gsettings, translations[i].gsettings_key);

                process_value (manager, &translations[i], val);
                g_variant_unref (val);
        }

        manager->priv->gsettings_font = g_settings_new (FONT_RENDER_SCHEMA);
        g_signal_connect (manager->priv->gsettings_font, "changed", G_CALLBACK (xft_callback), manager);
        
        /* Plugin settings (GTK modules and Xft) */
        manager->priv->plugin_settings = g_settings_new (XSETTINGS_PLUGIN_SCHEMA);
        g_signal_connect_object (manager->priv->plugin_settings, "changed", G_CALLBACK (plugin_callback), manager, 0);

        update_xft_settings (manager);

        start_fontconfig_monitor (manager);

        send_session_dbus();

        overrides = g_settings_get_value (manager->priv->plugin_settings, XSETTINGS_OVERRIDE_KEY);
        for (i = 0; manager->priv->managers [i]; i++){
                xsettings_manager_set_string (manager->priv->managers [i],
                                              "Net/FallbackIconTheme",
                                              "ukui");
                xsettings_manager_set_overrides (manager->priv->managers [i], overrides);
        }

        for (i = 0; manager->priv->managers [i]; i++) {
                xsettings_manager_notify (manager->priv->managers [i]);
        }

        g_variant_unref (overrides);
        ukui_settings_profile_end (NULL);

        return TRUE;
}

void
ukui_xsettings_manager_stop (UkuiXSettingsManager *manager)
{
        UkuiXSettingsManagerPrivate *p = manager->priv;
        int i;

        g_debug ("Stopping xsettings manager");

        if (p->managers != NULL) {
                for (i = 0; p->managers [i]; ++i)
                        xsettings_manager_destroy (p->managers [i]);

                g_free (p->managers);
                p->managers = NULL;
        }

        if (p->gsettings != NULL) {
                g_hash_table_destroy (p->gsettings);
                p->gsettings = NULL;
        }

        if (p->gsettings_font != NULL) {
                g_object_unref (p->gsettings_font);
                p->gsettings_font = NULL;
        }
        if (p->plugin_settings!=NULL) {
                g_object_unref (p->plugin_settings);
                p->plugin_settings = NULL;
        }

        stop_fontconfig_monitor (manager);

}

static GObject *
ukui_xsettings_manager_constructor(GType                    type,
                                   guint                    n_construct_properties,
                                   GObjectConstructParam    * construct_properties)
{
    UkuiXSettingsManager  *xsettings_manager;

    xsettings_manager = UKUI_XSETTINGS_MANAGER (G_OBJECT_CLASS(ukui_xsettings_manager_parent_class)->constructor (type,
                                                                                                                  n_construct_properties,
                                                                                                                  construct_properties));

    return G_OBJECT (xsettings_manager);

}

static void
ukui_xsettings_manager_class_init (UkuiXSettingsManagerClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        
        object_class->constructor = ukui_xsettings_manager_constructor;
        object_class->finalize = ukui_xsettings_manager_finalize;

        g_type_class_add_private (klass, sizeof (UkuiXSettingsManagerPrivate));
}

static void
ukui_xsettings_manager_init (UkuiXSettingsManager *manager)
{
        manager->priv = UKUI_XSETTINGS_MANAGER_GET_PRIVATE (manager);
}

static void
ukui_xsettings_manager_finalize (GObject *object)
{
        UkuiXSettingsManager *xsettings_manager;

        g_return_if_fail (object != NULL);
        g_return_if_fail (UKUI_IS_XSETTINGS_MANAGER (object));

        xsettings_manager = UKUI_XSETTINGS_MANAGER (object);

        g_return_if_fail (xsettings_manager->priv != NULL);

        G_OBJECT_CLASS (ukui_xsettings_manager_parent_class)->finalize (object);
}

UkuiXSettingsManager *
ukui_xsettings_manager_new (void)
{
        if (manager_object != NULL) {
                g_object_ref (manager_object);
        } else {
                manager_object = g_object_new (UKUI_TYPE_XSETTINGS_MANAGER, NULL);
                g_object_add_weak_pointer (manager_object,
                                           (gpointer *) &manager_object);
        }

        return UKUI_XSETTINGS_MANAGER (manager_object);
}
