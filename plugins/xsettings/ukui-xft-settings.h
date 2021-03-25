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
        double      window_scale;
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
