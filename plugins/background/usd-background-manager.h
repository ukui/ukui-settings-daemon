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

#ifndef __USD_BACKGROUND_MANAGER_H
#define __USD_BACKGROUND_MANAGER_H

#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif

//class UsdBackgroundManager
//{
	#define USD_TYPE_BACKGROUND_MANAGER         (usd_background_manager_get_type())
	#define USD_BACKGROUND_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST((o), USD_TYPE_BACKGROUND_MANAGER, UsdBackgroundManager))
	#define USD_BACKGROUND_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_TYPE_BACKGROUND_MANAGER, UsdBackgroundManagerClass))
	#define USD_IS_BACKGROUND_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE((o), USD_TYPE_BACKGROUND_MANAGER))
	#define USD_IS_BACKGROUND_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE((k), USD_TYPE_BACKGROUND_MANAGER))
	#define USD_BACKGROUND_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS((o), USD_TYPE_BACKGROUND_MANAGER, UsdBackgroundManagerClass))

	typedef struct UsdBackgroundManagerPrivate UsdBackgroundManagerPrivate;

	typedef struct {
		GObject                      parent;
		UsdBackgroundManagerPrivate* priv;
	} UsdBackgroundManager;

	typedef struct {
		GObjectClass   parent_class;
	} UsdBackgroundManagerClass;

	GType
	usd_background_manager_get_type (void);

	UsdBackgroundManager*
	usd_background_manager_new (void);

	gboolean
	usd_background_manager_start (UsdBackgroundManager* manager,
	                              GError**              error);

	void
	usd_background_manager_stop (UsdBackgroundManager* manager);
//}

#ifdef __cplusplus
}
#endif

#endif /* __USD_BACKGROUND_MANAGER_H */
