/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __USD_A11Y_SETTINGS_MANAGER_H
#define __USD_A11Y_SETTINGS_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define USD_TYPE_A11Y_SETTINGS_MANAGER         (usd_a11y_settings_manager_get_type ())
#define USD_A11Y_SETTINGS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_A11Y_SETTINGS_MANAGER, UsdA11ySettingsManager))
#define USD_A11Y_SETTINGS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_A11Y_SETTINGS_MANAGER, UsdA11ySettingsManagerClass))
#define USD_IS_A11Y_SETTINGS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_A11Y_SETTINGS_MANAGER))
#define USD_IS_A11Y_SETTINGS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_A11Y_SETTINGS_MANAGER))
#define USD_A11Y_SETTINGS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_A11Y_SETTINGS_MANAGER, UsdA11ySettingsManagerClass))

typedef struct UsdA11ySettingsManagerPrivate UsdA11ySettingsManagerPrivate;

typedef struct
{
        GObject                        parent;
        UsdA11ySettingsManagerPrivate *priv;
} UsdA11ySettingsManager;

typedef struct
{
        GObjectClass   parent_class;
} UsdA11ySettingsManagerClass;

GType                   usd_a11y_settings_manager_get_type            (void);

UsdA11ySettingsManager *usd_a11y_settings_manager_new                 (void);
gboolean                usd_a11y_settings_manager_start               (UsdA11ySettingsManager *manager,
                                                                       GError         **error);
void                    usd_a11y_settings_manager_stop                (UsdA11ySettingsManager *manager);

G_END_DECLS

#endif /* __USD_A11Y_SETTINGS_MANAGER_H */
