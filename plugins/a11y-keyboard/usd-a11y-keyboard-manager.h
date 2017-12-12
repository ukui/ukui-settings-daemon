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

#ifndef __USD_A11Y_KEYBOARD_MANAGER_H
#define __USD_A11Y_KEYBOARD_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_A11Y_KEYBOARD_MANAGER         (usd_a11y_keyboard_manager_get_type ())
#define USD_A11Y_KEYBOARD_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_A11Y_KEYBOARD_MANAGER, UsdA11yKeyboardManager))
#define USD_A11Y_KEYBOARD_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_A11Y_KEYBOARD_MANAGER, UsdA11yKeyboardManagerClass))
#define USD_IS_A11Y_KEYBOARD_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_A11Y_KEYBOARD_MANAGER))
#define USD_IS_A11Y_KEYBOARD_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_A11Y_KEYBOARD_MANAGER))
#define USD_A11Y_KEYBOARD_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_A11Y_KEYBOARD_MANAGER, UsdA11yKeyboardManagerClass))

typedef struct UsdA11yKeyboardManagerPrivate UsdA11yKeyboardManagerPrivate;

typedef struct
{
        GObject                     parent;
        UsdA11yKeyboardManagerPrivate *priv;
} UsdA11yKeyboardManager;

typedef struct
{
        GObjectClass   parent_class;
} UsdA11yKeyboardManagerClass;

GType                   usd_a11y_keyboard_manager_get_type            (void);

UsdA11yKeyboardManager *usd_a11y_keyboard_manager_new                 (void);
gboolean                usd_a11y_keyboard_manager_start               (UsdA11yKeyboardManager *manager,
                                                                       GError                **error);
void                    usd_a11y_keyboard_manager_stop                (UsdA11yKeyboardManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __USD_A11Y_KEYBOARD_MANAGER_H */
