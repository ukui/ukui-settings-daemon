/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 William Jon McCann <mccann@jhu.edu>
 * Copyright (C) 2020 KylinSoft Co., Ltd.
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __USD_SHARING_MANAGER_H
#define __USD_SHARING_MANAGER_H

#include <glib-object.h>

G_BEGIN_DECLS

#define USD_TYPE_SHARING_MANAGER         (usd_sharing_manager_get_type ())

G_DECLARE_FINAL_TYPE (UsdSharingManager, usd_sharing_manager, USD, SHARING_MANAGER, GObject)

typedef enum {
       USD_SHARING_STATUS_OFFLINE,
       USD_SHARING_STATUS_DISABLED_MOBILE_BROADBAND,
       USD_SHARING_STATUS_DISABLED_LOW_SECURITY,
       USD_SHARING_STATUS_AVAILABLE
} UsdSharingStatus;

UsdSharingManager *     usd_sharing_manager_new                 (void);
gboolean                usd_sharing_manager_start               (UsdSharingManager *manager,
                                                               GError         **error);
void                    usd_sharing_manager_stop                (UsdSharingManager *manager);

G_END_DECLS

#endif /* __USD_SHARING_MANAGER_H */
