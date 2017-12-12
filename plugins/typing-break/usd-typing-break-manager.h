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

#ifndef __USD_TYPING_BREAK_MANAGER_H
#define __USD_TYPING_BREAK_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_TYPING_BREAK_MANAGER         (usd_typing_break_manager_get_type ())
#define USD_TYPING_BREAK_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_TYPING_BREAK_MANAGER, UsdTypingBreakManager))
#define USD_TYPING_BREAK_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_TYPING_BREAK_MANAGER, UsdTypingBreakManagerClass))
#define USD_IS_TYPING_BREAK_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_TYPING_BREAK_MANAGER))
#define USD_IS_TYPING_BREAK_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_TYPING_BREAK_MANAGER))
#define USD_TYPING_BREAK_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_TYPING_BREAK_MANAGER, UsdTypingBreakManagerClass))

typedef struct UsdTypingBreakManagerPrivate UsdTypingBreakManagerPrivate;

typedef struct
{
        GObject                     parent;
        UsdTypingBreakManagerPrivate *priv;
} UsdTypingBreakManager;

typedef struct
{
        GObjectClass   parent_class;
} UsdTypingBreakManagerClass;

GType                   usd_typing_break_manager_get_type            (void);

UsdTypingBreakManager * usd_typing_break_manager_new                 (void);
gboolean                usd_typing_break_manager_start               (UsdTypingBreakManager *manager,
                                                                      GError               **error);
void                    usd_typing_break_manager_stop                (UsdTypingBreakManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __USD_TYPING_BREAK_MANAGER_H */
