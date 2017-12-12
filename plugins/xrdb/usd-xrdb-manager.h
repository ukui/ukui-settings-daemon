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

#ifndef __USD_XRDB_MANAGER_H
#define __USD_XRDB_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_TYPE_XRDB_MANAGER         (usd_xrdb_manager_get_type ())
#define USD_XRDB_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_TYPE_XRDB_MANAGER, UsdXrdbManager))
#define USD_XRDB_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_XRDB_MANAGER, UsdXrdbManagerClass))
#define USD_IS_XRDB_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_TYPE_XRDB_MANAGER))
#define USD_IS_XRDB_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_TYPE_XRDB_MANAGER))
#define USD_XRDB_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_TYPE_XRDB_MANAGER, UsdXrdbManagerClass))

typedef struct UsdXrdbManagerPrivate UsdXrdbManagerPrivate;

typedef struct
{
        GObject                     parent;
        UsdXrdbManagerPrivate *priv;
} UsdXrdbManager;

typedef struct
{
        GObjectClass   parent_class;
} UsdXrdbManagerClass;

GType                   usd_xrdb_manager_get_type            (void);

UsdXrdbManager *        usd_xrdb_manager_new                 (void);
gboolean                usd_xrdb_manager_start               (UsdXrdbManager *manager,
                                                              GError        **error);
void                    usd_xrdb_manager_stop                (UsdXrdbManager *manager);

#ifdef __cplusplus
}
#endif

#endif /* __USD_XRDB_MANAGER_H */
