/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 - Paolo Maggi
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

#ifndef UKUI_SETTINGS_MODULE_H
#define UKUI_SETTINGS_MODULE_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UKUI_TYPE_SETTINGS_MODULE               (ukui_settings_module_get_type ())
#define UKUI_SETTINGS_MODULE(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), UKUI_TYPE_SETTINGS_MODULE, UkuiSettingsModule))
#define UKUI_SETTINGS_MODULE_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), UKUI_TYPE_SETTINGS_MODULE, UkuiSettingsModuleClass))
#define UKUI_IS_SETTINGS_MODULE(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), UKUI_TYPE_SETTINGS_MODULE))
#define UKUI_IS_SETTINGS_MODULE_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((obj), UKUI_TYPE_SETTINGS_MODULE))
#define UKUI_SETTINGS_MODULE_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), UKUI_TYPE_SETTINGS_MODULE, UkuiSettingsModuleClass))

typedef struct _UkuiSettingsModule UkuiSettingsModule;

GType                    ukui_settings_module_get_type          (void) G_GNUC_CONST;

UkuiSettingsModule     *ukui_settings_module_new               (const gchar *path);

const char              *ukui_settings_module_get_path          (UkuiSettingsModule *module);

GObject                 *ukui_settings_module_new_object        (UkuiSettingsModule *module);

#ifdef __cplusplus
}
#endif

#endif
