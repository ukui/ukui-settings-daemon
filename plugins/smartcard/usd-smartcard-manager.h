/* usd-smartcard-manager.h - object for monitoring smartcard insertion and
 *                           removal events
 *
 * Copyright (C) 2006, 2009 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Written by: Ray Strode
 */
#ifndef USD_SMARTCARD_MANAGER_H
#define USD_SMARTCARD_MANAGER_H

#define USD_SMARTCARD_ENABLE_INTERNAL_API
#include "usd-smartcard.h"

#include <glib.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif
#define USD_TYPE_SMARTCARD_MANAGER            (usd_smartcard_manager_get_type ())
#define USD_SMARTCARD_MANAGER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), USD_TYPE_SMARTCARD_MANAGER, UsdSmartcardManager))
#define USD_SMARTCARD_MANAGER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), USD_TYPE_SMARTCARD_MANAGER, UsdSmartcardManagerClass))
#define USD_IS_SMARTCARD_MANAGER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SC_TYPE_SMARTCARD_MANAGER))
#define USD_IS_SMARTCARD_MANAGER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SC_TYPE_SMARTCARD_MANAGER))
#define USD_SMARTCARD_MANAGER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), USD_TYPE_SMARTCARD_MANAGER, UsdSmartcardManagerClass))
#define USD_SMARTCARD_MANAGER_ERROR           (usd_smartcard_manager_error_quark ())
typedef struct _UsdSmartcardManager UsdSmartcardManager;
typedef struct _UsdSmartcardManagerClass UsdSmartcardManagerClass;
typedef struct _UsdSmartcardManagerPrivate UsdSmartcardManagerPrivate;
typedef enum _UsdSmartcardManagerError UsdSmartcardManagerError;

struct _UsdSmartcardManager {
    GObject parent;

    /*< private > */
    UsdSmartcardManagerPrivate *priv;
};

struct _UsdSmartcardManagerClass {
        GObjectClass parent_class;

        /* Signals */
        void (*smartcard_inserted) (UsdSmartcardManager *manager,
                                    UsdSmartcard        *token);
        void (*smartcard_removed) (UsdSmartcardManager *manager,
                                   UsdSmartcard        *token);
        void (*error) (UsdSmartcardManager *manager,
                       GError              *error);
};

enum _UsdSmartcardManagerError {
    USD_SMARTCARD_MANAGER_ERROR_GENERIC = 0,
    USD_SMARTCARD_MANAGER_ERROR_WITH_NSS,
    USD_SMARTCARD_MANAGER_ERROR_LOADING_DRIVER,
    USD_SMARTCARD_MANAGER_ERROR_WATCHING_FOR_EVENTS,
    USD_SMARTCARD_MANAGER_ERROR_REPORTING_EVENTS
};

GType usd_smartcard_manager_get_type (void) G_GNUC_CONST;
GQuark usd_smartcard_manager_error_quark (void) G_GNUC_CONST;

UsdSmartcardManager *usd_smartcard_manager_new (const char *module);

gboolean usd_smartcard_manager_start (UsdSmartcardManager  *manager,
                                      GError              **error);

void usd_smartcard_manager_stop (UsdSmartcardManager *manager);

char *usd_smartcard_manager_get_module_path (UsdSmartcardManager *manager);
gboolean usd_smartcard_manager_login_card_is_inserted (UsdSmartcardManager *manager);

#ifdef __cplusplus
}
#endif
#endif                                /* USD_SMARTCARD_MANAGER_H */
