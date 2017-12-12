/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2002-2005 Paolo Maggi
 * Copyright (C) 2007      William Jon McCann <mccann@jhu.edu>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "ukui-settings-plugin.h"

G_DEFINE_TYPE (UkuiSettingsPlugin, ukui_settings_plugin, G_TYPE_OBJECT)

static void
dummy (UkuiSettingsPlugin *plugin)
{
        /* Empty */
}

static void
ukui_settings_plugin_class_init (UkuiSettingsPluginClass *klass)
{
        klass->activate = dummy;
        klass->deactivate = dummy;
}

static void
ukui_settings_plugin_init (UkuiSettingsPlugin *plugin)
{
        /* Empty */
}

void
ukui_settings_plugin_activate (UkuiSettingsPlugin *plugin)
{
        g_return_if_fail (UKUI_IS_SETTINGS_PLUGIN (plugin));

        UKUI_SETTINGS_PLUGIN_GET_CLASS (plugin)->activate (plugin);
}

void
ukui_settings_plugin_deactivate  (UkuiSettingsPlugin *plugin)
{
        g_return_if_fail (UKUI_IS_SETTINGS_PLUGIN (plugin));

        UKUI_SETTINGS_PLUGIN_GET_CLASS (plugin)->deactivate (plugin);
}
