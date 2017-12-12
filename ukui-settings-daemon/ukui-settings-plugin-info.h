/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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

#ifndef __UKUI_SETTINGS_PLUGIN_INFO_H__
#define __UKUI_SETTINGS_PLUGIN_INFO_H__

#include <glib-object.h>
#include <gmodule.h>

#ifdef __cplusplus
extern "C" {
#endif
#define UKUI_TYPE_SETTINGS_PLUGIN_INFO              (ukui_settings_plugin_info_get_type())
#define UKUI_SETTINGS_PLUGIN_INFO(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), UKUI_TYPE_SETTINGS_PLUGIN_INFO, UkuiSettingsPluginInfo))
#define UKUI_SETTINGS_PLUGIN_INFO_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass),  UKUI_TYPE_SETTINGS_PLUGIN_INFO, UkuiSettingsPluginInfoClass))
#define UKUI_IS_SETTINGS_PLUGIN_INFO(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), UKUI_TYPE_SETTINGS_PLUGIN_INFO))
#define UKUI_IS_SETTINGS_PLUGIN_INFO_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), UKUI_TYPE_SETTINGS_PLUGIN_INFO))
#define UKUI_SETTINGS_PLUGIN_INFO_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj),  UKUI_TYPE_SETTINGS_PLUGIN_INFO, UkuiSettingsPluginInfoClass))

typedef struct UkuiSettingsPluginInfoPrivate UkuiSettingsPluginInfoPrivate;

typedef struct
{
        GObject                         parent;
        UkuiSettingsPluginInfoPrivate *priv;
} UkuiSettingsPluginInfo;

typedef struct
{
        GObjectClass parent_class;

        void          (* activated)         (UkuiSettingsPluginInfo *info);
        void          (* deactivated)       (UkuiSettingsPluginInfo *info);
} UkuiSettingsPluginInfoClass;

GType            ukui_settings_plugin_info_get_type           (void) G_GNUC_CONST;

UkuiSettingsPluginInfo *ukui_settings_plugin_info_new_from_file (const char *filename);

gboolean         ukui_settings_plugin_info_activate        (UkuiSettingsPluginInfo *info);
gboolean         ukui_settings_plugin_info_deactivate      (UkuiSettingsPluginInfo *info);

gboolean         ukui_settings_plugin_info_is_active       (UkuiSettingsPluginInfo *info);
gboolean         ukui_settings_plugin_info_get_enabled     (UkuiSettingsPluginInfo *info);
gboolean         ukui_settings_plugin_info_is_available    (UkuiSettingsPluginInfo *info);

const char      *ukui_settings_plugin_info_get_name        (UkuiSettingsPluginInfo *info);
const char      *ukui_settings_plugin_info_get_description (UkuiSettingsPluginInfo *info);
const char     **ukui_settings_plugin_info_get_authors     (UkuiSettingsPluginInfo *info);
const char      *ukui_settings_plugin_info_get_website     (UkuiSettingsPluginInfo *info);
const char      *ukui_settings_plugin_info_get_copyright   (UkuiSettingsPluginInfo *info);
const char      *ukui_settings_plugin_info_get_location    (UkuiSettingsPluginInfo *info);
int              ukui_settings_plugin_info_get_priority    (UkuiSettingsPluginInfo *info);

void             ukui_settings_plugin_info_set_priority    (UkuiSettingsPluginInfo *info,
                                                            int                     priority);
void             ukui_settings_plugin_info_set_schema      (UkuiSettingsPluginInfo *info,
                                                            gchar                  *schema);
#ifdef __cplusplus
}
#endif

#endif  /* __UKUI_SETTINGS_PLUGIN_INFO_H__ */
