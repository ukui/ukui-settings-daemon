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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef __UKUI_XSETTINGS_MANAGER_H
#define __UKUI_XSETTINGS_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UKUI_TYPE_XSETTINGS_MANAGER         (ukui_xsettings_manager_get_type ())
#define UKUI_XSETTINGS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), UKUI_TYPE_XSETTINGS_MANAGER, UkuiXSettingsManager))
#define UKUI_XSETTINGS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), UKUI_TYPE_XSETTINGS_MANAGER, UkuiXSettingsManagerClass))
#define UKUI_IS_XSETTINGS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), UKUI_TYPE_XSETTINGS_MANAGER))
#define UKUI_IS_XSETTINGS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), UKUI_TYPE_XSETTINGS_MANAGER))
#define UKUI_XSETTINGS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), UKUI_TYPE_XSETTINGS_MANAGER, UkuiXSettingsManagerClass))

typedef struct UkuiXSettingsManagerPrivate UkuiXSettingsManagerPrivate;

typedef struct
{
        GObject                     parent;
        UkuiXSettingsManagerPrivate *priv;
} UkuiXSettingsManager;

typedef struct
{
        GObjectClass   parent_class;
} UkuiXSettingsManagerClass;

GType                   ukui_xsettings_manager_get_type            (void);

UkuiXSettingsManager * ukui_xsettings_manager_new                 (void);
gboolean                ukui_xsettings_manager_start               (UkuiXSettingsManager *manager,
                                                                     GError               **error);
void                    ukui_xsettings_manager_stop                (UkuiXSettingsManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __UKUI_XSETTINGS_MANAGER_H */
