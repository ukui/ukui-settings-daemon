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

#include "config.h"

#include "ukui-settings-module.h"

#include <gmodule.h>

typedef struct _UkuiSettingsModuleClass UkuiSettingsModuleClass;

struct _UkuiSettingsModuleClass
{
        GTypeModuleClass parent_class;
};

struct _UkuiSettingsModule
{
        GTypeModule parent_instance;

        GModule    *library;

        char       *path;
        GType       type;
};

typedef GType (*UkuiSettingsModuleRegisterFunc) (GTypeModule *);

G_DEFINE_TYPE (UkuiSettingsModule, ukui_settings_module, G_TYPE_TYPE_MODULE)

static gboolean
ukui_settings_module_load (GTypeModule *gmodule)
{
        UkuiSettingsModule            *module;
        UkuiSettingsModuleRegisterFunc register_func;
        gboolean                        res;

        module = UKUI_SETTINGS_MODULE (gmodule);

        g_debug ("Loading %s", module->path);

        module->library = g_module_open (module->path, 0);

        if (module->library == NULL) {
                g_warning ("%s", g_module_error ());

                return FALSE;
        }

        /* extract symbols from the lib */
        res = g_module_symbol (module->library, "register_ukui_settings_plugin", (void *) &register_func);
        if (! res) {
                g_warning ("%s", g_module_error ());
                g_module_close (module->library);

                return FALSE;
        }

        g_assert (register_func);

        module->type = register_func (gmodule);

        if (module->type == 0) {
                g_warning ("Invalid ukui settings plugin in module %s", module->path);
                return FALSE;
        }

        return TRUE;
}

static void
ukui_settings_module_unload (GTypeModule *gmodule)
{
        UkuiSettingsModule *module = UKUI_SETTINGS_MODULE (gmodule);

        g_debug ("Unloading %s", module->path);

        g_module_close (module->library);

        module->library = NULL;
        module->type = 0;
}

const gchar *
ukui_settings_module_get_path (UkuiSettingsModule *module)
{
        g_return_val_if_fail (UKUI_IS_SETTINGS_MODULE (module), NULL);

        return module->path;
}

GObject *
ukui_settings_module_new_object (UkuiSettingsModule *module)
{
        g_debug ("Creating object of type %s", g_type_name (module->type));

        if (module->type == 0) {
                return NULL;
        }

        return g_object_new (module->type, NULL);
}

static void
ukui_settings_module_init (UkuiSettingsModule *module)
{
        g_debug ("UkuiSettingsModule %p initialising", module);
}

static void
ukui_settings_module_finalize (GObject *object)
{
        UkuiSettingsModule *module = UKUI_SETTINGS_MODULE (object);

        g_debug ("UkuiSettingsModule %p finalizing", module);

        g_free (module->path);

        G_OBJECT_CLASS (ukui_settings_module_parent_class)->finalize (object);
}

static void
ukui_settings_module_class_init (UkuiSettingsModuleClass *class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (class);
        GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

        object_class->finalize = ukui_settings_module_finalize;

        module_class->load = ukui_settings_module_load;
        module_class->unload = ukui_settings_module_unload;
}

UkuiSettingsModule *
ukui_settings_module_new (const char *path)
{
        UkuiSettingsModule *result;

        if (path == NULL || path[0] == '\0') {
                return NULL;
        }

        result = g_object_new (UKUI_TYPE_SETTINGS_MODULE, NULL);

        g_type_module_set_name (G_TYPE_MODULE (result), path);
        result->path = g_strdup (path);

        return result;
}
