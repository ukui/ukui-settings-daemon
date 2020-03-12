
#include "xsettings-const.h"
#include "ukui-xft-settings.h"
#include "ukui-xsettings-manager.h"
#include <gio/gio.h>
#include <glib.h>


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


static double
get_dpi_from_x_server (void)
{
        GdkScreen *screen;
        double     dpi;

        screen = gdk_screen_get_default ();
        if (screen != NULL) {
                double width_dpi, height_dpi;

                width_dpi = dpi_from_pixels_and_mm (gdk_screen_get_width (screen), gdk_screen_get_width_mm (screen));
                height_dpi = dpi_from_pixels_and_mm (gdk_screen_get_height (screen), gdk_screen_get_height_mm (screen));

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
                dpi = get_dpi_from_x_server ();
        }

        return dpi;
}



void UkuiXftSettings::xft_settings_set_xsettings (ukuiXSettingsManager *manager)
{
    int i;
    //ukui_settings_profile_start (NULL);
    for (i = 0; manager->pManagers [i]; i++) {
       manager->pManagers [i]->set_int ("Xft/Antialias", antialias);
       manager->pManagers [i]->set_int ("Xft/Hinting", hinting);
       manager->pManagers [i]->set_string ("Xft/HintStyle", hintstyle);
       manager->pManagers [i]->set_int ("Xft/DPI", dpi);
       manager->pManagers [i]->set_string ("Xft/RGBA", rgba);
       manager->pManagers [i]->set_string ("Xft/lcdfilter",
                                     g_str_equal (rgba, "rgb") ? "lcddefault" : "none");
       manager->pManagers [i]->set_int ("Gtk/CursorThemeSize", cursor_size);
       manager->pManagers [i]->set_string ("Gtk/CursorThemeName", cursor_theme);
    }
    //ukui_settings_profile_end (NULL);
}

void UkuiXftSettings::xft_settings_get (ukuiXSettingsManager *manager)
{
    GSettings *mouse_gsettings;
    char      *antialiasing;
    char      *hinting;
    char      *rgba_order;
    double     dpi;

    mouse_gsettings = (GSettings *)g_hash_table_lookup (manager->gsettings, (void*)MOUSE_SCHEMA);

    antialiasing = g_settings_get_string (manager->gsettings_font, FONT_ANTIALIASING_KEY);
    hinting = g_settings_get_string (manager->gsettings_font, FONT_HINTING_KEY);
    rgba_order = g_settings_get_string (manager->gsettings_font, FONT_RGBA_ORDER_KEY);
    dpi = get_dpi_from_gsettings_or_x_server (manager->gsettings_font);

    antialias = TRUE;
    this->hinting = TRUE;
    hintstyle = "hintslight";
    dpi = dpi * 1024; /* Xft wants 1/1024ths of an inch */
    cursor_theme = g_settings_get_string (mouse_gsettings, CURSOR_THEME_KEY);
    cursor_size = g_settings_get_int (mouse_gsettings, CURSOR_SIZE_KEY);
    rgba = "rgb";

    if (rgba_order) {
        int i;
        gboolean found = FALSE;

        for (i = 0; i < G_N_ELEMENTS (rgba_types) && !found; i++) {
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

    //ukui_settings_profile_start (NULL);

    /* get existing properties */
    dpy = XOpenDisplay (NULL);
    g_return_if_fail (dpy != NULL);
    add_string = g_string_new (XResourceManagerString (dpy));
    g_debug("xft_settings_set_xresources: orig res '%s'", add_string->str);
    update_property (add_string, "Xft.dpi",
                            g_ascii_dtostr (dpibuf, sizeof (dpibuf), (double) this->dpi / 1024.0));
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
    XCloseDisplay (dpy);
    g_string_free (add_string, TRUE);
    // ukui_settings_profile_end (NULL);
    }
