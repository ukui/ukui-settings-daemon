/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2007 David Zeuthen <david@fubar.dk>
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

#ifndef USD_DATETIME_MECHANISM_H
#define USD_DATETIME_MECHANISM_H

#include <glib-object.h>
#include <dbus/dbus-glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define USD_DATETIME_TYPE_MECHANISM         (usd_datetime_mechanism_get_type ())
#define USD_DATETIME_MECHANISM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), USD_DATETIME_TYPE_MECHANISM, UsdDatetimeMechanism))
#define USD_DATETIME_MECHANISM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), USD_DATETIME_TYPE_MECHANISM, UsdDatetimeMechanismClass))
#define USD_DATETIME_IS_MECHANISM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), USD_DATETIME_TYPE_MECHANISM))
#define USD_DATETIME_IS_MECHANISM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), USD_DATETIME_TYPE_MECHANISM))
#define USD_DATETIME_MECHANISM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), USD_DATETIME_TYPE_MECHANISM, UsdDatetimeMechanismClass))

typedef struct UsdDatetimeMechanismPrivate UsdDatetimeMechanismPrivate;

typedef struct
{
        GObject        parent;
        UsdDatetimeMechanismPrivate *priv;
} UsdDatetimeMechanism;

typedef struct
{
        GObjectClass   parent_class;
} UsdDatetimeMechanismClass;

typedef enum
{
        USD_DATETIME_MECHANISM_ERROR_GENERAL,
        USD_DATETIME_MECHANISM_ERROR_NOT_PRIVILEGED,
        USD_DATETIME_MECHANISM_ERROR_INVALID_TIMEZONE_FILE,
        USD_DATETIME_MECHANISM_NUM_ERRORS
} UsdDatetimeMechanismError;

#define USD_DATETIME_MECHANISM_ERROR usd_datetime_mechanism_error_quark ()

GType usd_datetime_mechanism_error_get_type (void);
#define USD_DATETIME_MECHANISM_TYPE_ERROR (usd_datetime_mechanism_error_get_type ())


GQuark                     usd_datetime_mechanism_error_quark         (void);
GType                      usd_datetime_mechanism_get_type            (void);
UsdDatetimeMechanism      *usd_datetime_mechanism_new                 (void);

/* exported methods */
gboolean            usd_datetime_mechanism_get_timezone (UsdDatetimeMechanism   *mechanism,
                                                         DBusGMethodInvocation  *context);
gboolean            usd_datetime_mechanism_set_timezone (UsdDatetimeMechanism   *mechanism,
                                                         const char             *zone_file,
                                                         DBusGMethodInvocation  *context);

gboolean            usd_datetime_mechanism_can_set_timezone (UsdDatetimeMechanism  *mechanism,
                                                             DBusGMethodInvocation *context);

gboolean            usd_datetime_mechanism_set_time     (UsdDatetimeMechanism  *mechanism,
                                                         gint64                 seconds_since_epoch,
                                                         DBusGMethodInvocation *context);

gboolean            usd_datetime_mechanism_can_set_time (UsdDatetimeMechanism  *mechanism,
                                                         DBusGMethodInvocation *context);

gboolean            usd_datetime_mechanism_adjust_time  (UsdDatetimeMechanism  *mechanism,
                                                         gint64                 seconds_to_add,
                                                         DBusGMethodInvocation *context);

gboolean            usd_datetime_mechanism_get_hardware_clock_using_utc  (UsdDatetimeMechanism  *mechanism,
                                                                          DBusGMethodInvocation *context);

gboolean            usd_datetime_mechanism_set_hardware_clock_using_utc  (UsdDatetimeMechanism  *mechanism,
                                                                          gboolean               using_utc,
                                                                          DBusGMethodInvocation *context);

#ifdef __cplusplus
}
#endif

#endif /* USD_DATETIME_MECHANISM_H */
