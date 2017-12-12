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

#ifndef __USD_KEYBINDINGS_MANAGER_H
#define __USD_KEYBINDINGS_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_KEYBINDINGS_MANAGER         (usd_keybindings_manager_get_type ())
#define USD_KEYBINDINGS_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_KEYBINDINGS_MANAGER, UsdKeybindingsManager))
#define USD_KEYBINDINGS_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_KEYBINDINGS_MANAGER, UsdKeybindingsManagerClass))
#define USD_IS_KEYBINDINGS_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_KEYBINDINGS_MANAGER))
#define USD_IS_KEYBINDINGS_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_KEYBINDINGS_MANAGER))
#define USD_KEYBINDINGS_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_KEYBINDINGS_MANAGER, UsdKeybindingsManagerClass))

typedef struct UsdKeybindingsManagerPrivate UsdKeybindingsManagerPrivate;

typedef struct
{
        GObject                     parent;
        UsdKeybindingsManagerPrivate *priv;
} UsdKeybindingsManager;

typedef struct
{
        GObjectClass   parent_class;
} UsdKeybindingsManagerClass;

GType                   usd_keybindings_manager_get_type            (void);

UsdKeybindingsManager *       usd_keybindings_manager_new                 (void);
gboolean                usd_keybindings_manager_start               (UsdKeybindingsManager *manager,
                                                               GError         **error);
void                    usd_keybindings_manager_stop                (UsdKeybindingsManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __USD_KEYBINDINGS_MANAGER_H */
