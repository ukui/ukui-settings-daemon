#ifndef UKUIXFTSETTINGS_H
#define UKUIXFTSETTINGS_H

#include <glib.h>
class ukuiXSettingsManager;

class UkuiXftSettings
{
private:
        gboolean    antialias;
        gboolean    hinting;
        int         dpi;
        int         scaled_dpi;
        int         window_scale;
        char       *cursor_theme;
        int         cursor_size;
        const char *rgba;
        const char *hintstyle;
public:
        void xft_settings_get (ukuiXSettingsManager *manager);
        void xft_settings_set_xsettings (ukuiXSettingsManager *manager);
        void xft_settings_set_xresources ();
};

#endif // UKUIXFTSETTINGS_H
