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
#ifndef UKUIXSETTINGSMANAGER_H
#define UKUIXSETTINGSMANAGER_H

#include <glib.h>
#include <X11/Xatom.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/extensions/Xfixes.h>

#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gio/gio.h>
//#include "ixsettings-manager.h"
#include "xsettings-manager.h"
#include "fontconfig-monitor.h"


class ukuiXSettingsManager
{
public:
    ukuiXSettingsManager();
    ~ukuiXSettingsManager();
    bool start();
    int stop();

    void sendSessionDbus();

    //gboolean setup_xsettings_managers (ukuiXSettingsManager *manager);
    XsettingsManager **pManagers;
    GHashTable  *gsettings;
    GSettings   *gsettings_font;
    //GSettings   *plugin_settings;
    fontconfig_monitor_handle_t *fontconfig_handle;
};

#endif // UKUIXSETTINGSMANAGER_H
