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
#include "xsettings-const.h"
#include "ukui-xft-settings.h"
#include "ukui-xsettings-manager.h"
#include <gio/gio.h>
#include <glib.h>
#include <gdk/gdkx.h>

static const char *rgba_types[] = { "rgb", "bgr", "vbgr", "vrgb" };

static void update_property (GString *props, const gchar* key, const gchar* value)
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

static double dpi_from_pixels_and_mm (int pixels, int mm)
{
    double dpi;
    if (mm >= 1)
        dpi = pixels / (mm / 25.4);
    else
        dpi = 0;
    return dpi;
}

static double get_dpi_from_x_server (void)
{
    GdkScreen *screen;
    double     dpi;

    screen = gdk_screen_get_default ();
    if (screen != NULL) {
        double width_dpi, height_dpi;
        Screen *xscreen = gdk_x11_screen_get_xscreen (screen);

        width_dpi = dpi_from_pixels_and_mm (WidthOfScreen (xscreen), WidthMMOfScreen (xscreen));
        height_dpi = dpi_from_pixels_and_mm (HeightOfScreen (xscreen), HeightMMOfScreen (xscreen));

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

static double get_dpi_from_gsettings_or_x_server (GSettings *gsettings)
{
    double value;
    double dpi;

    value = g_settings_get_double (gsettings, FONT_DPI_KEY);

    /* If the user has ever set the DPI preference in GSettings, we use that.
     * Otherwise, we see if the X server reports a reasonable DPI value:  some X
     * servers report completely bogus values, and the user gets huge or tiny
     * fonts which are unusable.
     */

    if (value != 0) {
        dpi = value;
    } else {
        dpi = DPI_FALLBACK;//get_dpi_from_x_server ();
    }

    return dpi;
}

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
static int
get_window_scale (ukuiXSettingsManager *manager)
{
    GSettings   *gsettings;
    int         scale;
    gsettings = (GSettings *)g_hash_table_lookup(manager->gsettings,XSETTINGS_PLUGIN_SCHEMA);
    scale = g_settings_get_int (gsettings, SCALING_FACTOR_KEY);
    /* Auto-detect */
    if (scale == 0)
        scale = get_window_scale_auto ();
    return scale;
}

void UkuiXftSettings::xft_settings_set_xsettings (ukuiXSettingsManager *manager)
{
    int i;

    for (i = 0; manager->pManagers [i]; i++) {
        manager->pManagers [i]->set_int ("Xft/Antialias", antialias);
        manager->pManagers [i]->set_int ("Xft/Hinting", hinting);
        manager->pManagers [i]->set_string ("Xft/HintStyle", hintstyle);

        manager->pManagers [i]->set_int ( "Gdk/WindowScalingFactor",window_scale);
        manager->pManagers [i]->set_int ("Gdk/UnscaledDPI",dpi);
        manager->pManagers [i]->set_int ("Xft/DPI", scaled_dpi);

        manager->pManagers [i]->set_string ("Xft/RGBA", rgba);
        manager->pManagers [i]->set_string ("Xft/lcdfilter",
                 g_str_equal (rgba, "rgb") ? "lcddefault" : "none");
        manager->pManagers [i]->set_int ("Gtk/CursorThemeSize", cursor_size);
        manager->pManagers [i]->set_string ("Gtk/CursorThemeName", cursor_theme);
    }
}

void UkuiXftSettings::xft_settings_get (ukuiXSettingsManager *manager)
{
    GSettings *mouse_gsettings;
    char      *antialiasing;
    char      *hinting;
    char      *rgba_order;
    double     dpi;
    int        scale;

    mouse_gsettings = (GSettings *)g_hash_table_lookup (manager->gsettings, (void*)MOUSE_SCHEMA);

    antialiasing = g_settings_get_string (manager->gsettings_font, FONT_ANTIALIASING_KEY);
    hinting = g_settings_get_string (manager->gsettings_font, FONT_HINTING_KEY);
    rgba_order = g_settings_get_string (manager->gsettings_font, FONT_RGBA_ORDER_KEY);
    dpi = get_dpi_from_gsettings_or_x_server (manager->gsettings_font);
    scale = get_window_scale (manager);

    antialias = TRUE;
    this->hinting = TRUE;
    hintstyle = "hintslight";

    this->window_scale = scale;
    this->dpi = dpi * 1024; /* Xft wants 1/1024ths of an inch */
    this->scaled_dpi = dpi * scale * 1024;

    cursor_theme = g_settings_get_string (mouse_gsettings, CURSOR_THEME_KEY);
    cursor_size = g_settings_get_int (mouse_gsettings, CURSOR_SIZE_KEY);
    rgba = "rgb";

    if (rgba_order) {
        int i;
        gboolean found = FALSE;

        for (i = 0; i < (int)G_N_ELEMENTS (rgba_types) && !found; i++) {
            if (strcmp (rgba_order, rgba_types[i]) == 0) {
                rgba = rgba_types[i];
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
            this->hinting = 0;
            hintstyle = "hintnone";
        } else if (strcmp (hinting, "slight") == 0) {
            this->hinting = 1;
            hintstyle = "hintslight";
        } else if (strcmp (hinting, "medium") == 0) {
            this->hinting = 1;
            hintstyle = "hintmedium";
        } else if (strcmp (hinting, "full") == 0) {
            this->hinting = 1;
            hintstyle = "hintfull";
        } else {
            g_warning ("Invalid value for " FONT_HINTING_KEY ": '%s'",
                    hinting);
        }
    }

    if (antialiasing) {
        gboolean use_rgba = FALSE;
        if (strcmp (antialiasing, "none") == 0) {
            antialias = 0;
        } else if (strcmp (antialiasing, "grayscale") == 0) {
            antialias = 1;
        } else if (strcmp (antialiasing, "rgba") == 0) {
            antialias = 1;
            use_rgba = TRUE;
        } else {
            g_warning ("Invalid value for " FONT_ANTIALIASING_KEY " : '%s'",
                    antialiasing);
        }
        if (!use_rgba) {
            rgba = "none";
        }
    }

    g_free (rgba_order);
    g_free (hinting);
    g_free (antialiasing);
}

void UkuiXftSettings::xft_settings_set_xresources ()
{
    GString    *add_string;
    char        dpibuf[G_ASCII_DTOSTR_BUF_SIZE];
    Display    *dpy;


    /* get existing properties */
    dpy = XOpenDisplay (NULL);
    g_return_if_fail (dpy != NULL);
    add_string = g_string_new (XResourceManagerString (dpy));
    g_debug("xft_settings_set_xresources: orig res '%s'", add_string->str);
    
    char tmpCursorTheme[255] = {'\0'};
    int tmpCursorSize = 0;
    if (strlen (this->cursor_theme) > 0) {
        strncpy(tmpCursorTheme, this->cursor_theme, 255);
    } else {
        // unset, use default
        strncpy(tmpCursorTheme, "DMZ-Black", 255);
    }
    if (this->cursor_size > 0) {
        tmpCursorSize = this->cursor_size;
    } else {
        tmpCursorSize = XcursorGetDefaultSize(dpy);
    }

    update_property (add_string, "Xft.dpi",
            g_ascii_dtostr (dpibuf, sizeof (dpibuf), (double) this->scaled_dpi / 1024.0));
    update_property (add_string, "Xft.antialias",
            this->antialias ? "1" : "0");
    update_property (add_string, "Xft.hinting",
            this->hinting ? "1" : "0");
    update_property (add_string, "Xft.hintstyle",
            this->hintstyle);
    update_property (add_string, "Xft.rgba",
            this->rgba);
    update_property (add_string, "Xft.lcdfilter",
            g_str_equal (this->rgba, "rgb") ? "lcddefault" : "none");
    update_property (add_string, "Xcursor.theme",
            this->cursor_theme);
    update_property (add_string, "Xcursor.size",
            g_ascii_dtostr (dpibuf, sizeof (dpibuf), (double) this->cursor_size));
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
                    gdk_x11_display_set_cursor_theme(gdk_display_get_default(),
                                                     CursorsNames[i],
                                                     cursor_size);
                }
            }
            XFixesChangeCursorByName(dpy, handle, CursorsNames[i]);
            XcursorImagesDestroy(images);
        }
    }
    // end add
    GdkCursor *cursor = gdk_cursor_new_for_display(gdk_display_get_default(),
		                                   GDK_LEFT_PTR);
    gdk_window_set_cursor(gdk_get_default_root_window(), cursor);

    g_object_unref(G_OBJECT(cursor));
    XCloseDisplay (dpy);
    g_string_free (add_string, TRUE);
}
