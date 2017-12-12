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

#ifndef __USD_KEYBOARD_MANAGER_H
#define __USD_KEYBOARD_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_KEYBOARD_MANAGER         (usd_keyboard_manager_get_type ())
#define USD_KEYBOARD_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_KEYBOARD_MANAGER, UsdKeyboardManager))
#define USD_KEYBOARD_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_KEYBOARD_MANAGER, UsdKeyboardManagerClass))
#define USD_IS_KEYBOARD_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_KEYBOARD_MANAGER))
#define USD_IS_KEYBOARD_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_KEYBOARD_MANAGER))
#define USD_KEYBOARD_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_KEYBOARD_MANAGER, UsdKeyboardManagerClass))

typedef struct UsdKeyboardManagerPrivate UsdKeyboardManagerPrivate;

typedef struct
{
        GObject                     parent;
        UsdKeyboardManagerPrivate *priv;
} UsdKeyboardManager;

typedef struct
{
        GObjectClass   parent_class;
} UsdKeyboardManagerClass;

GType                   usd_keyboard_manager_get_type            (void);

UsdKeyboardManager *       usd_keyboard_manager_new                 (void);
gboolean                usd_keyboard_manager_start               (UsdKeyboardManager *manager,
                                                               GError         **error);
void                    usd_keyboard_manager_stop                (UsdKeyboardManager *manager);
void                    usd_keyboard_manager_apply_settings      (UsdKeyboardManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __USD_KEYBOARD_MANAGER_H */
