/* securitycard.h - api for reading and writing data to a security card
 *
 * Copyright (C) 2006 Ray Strode
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
 */
#ifndef USD_SMARTCARD_H
#define USD_SMARTCARD_H

#include <glib.h>
#include <glib-object.h>

#include <secmod.h>

#ifdef __cplusplus
extern "C" {
#endif
#define USD_TYPE_SMARTCARD            (usd_smartcard_get_type ())
#define USD_SMARTCARD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), USD_TYPE_SMARTCARD, UsdSmartcard))
#define USD_SMARTCARD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), USD_TYPE_SMARTCARD, UsdSmartcardClass))
#define USD_IS_SMARTCARD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), USD_TYPE_SMARTCARD))
#define USD_IS_SMARTCARD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), USD_TYPE_SMARTCARD))
#define USD_SMARTCARD_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), USD_TYPE_SMARTCARD, UsdSmartcardClass))
#define USD_SMARTCARD_ERROR           (usd_smartcard_error_quark ())
typedef struct _UsdSmartcardClass UsdSmartcardClass;
typedef struct _UsdSmartcard UsdSmartcard;
typedef struct _UsdSmartcardPrivate UsdSmartcardPrivate;
typedef enum _UsdSmartcardError UsdSmartcardError;
typedef enum _UsdSmartcardState UsdSmartcardState;

typedef struct _UsdSmartcardRequest UsdSmartcardRequest;

struct _UsdSmartcard {
    GObject parent;

    /*< private > */
    UsdSmartcardPrivate *priv;
};

struct _UsdSmartcardClass {
    GObjectClass parent_class;

    void (* inserted) (UsdSmartcard *card);
    void (* removed)  (UsdSmartcard *card);
};

enum _UsdSmartcardError {
    USD_SMARTCARD_ERROR_GENERIC = 0,
};

enum _UsdSmartcardState {
    USD_SMARTCARD_STATE_INSERTED = 0,
    USD_SMARTCARD_STATE_REMOVED,
};

GType usd_smartcard_get_type (void) G_GNUC_CONST;
GQuark usd_smartcard_error_quark (void) G_GNUC_CONST;

CK_SLOT_ID usd_smartcard_get_slot_id (UsdSmartcard *card);
gint usd_smartcard_get_slot_series (UsdSmartcard *card);
UsdSmartcardState usd_smartcard_get_state (UsdSmartcard *card);

char *usd_smartcard_get_name (UsdSmartcard *card);
gboolean usd_smartcard_is_login_card (UsdSmartcard *card);

gboolean usd_smartcard_unlock (UsdSmartcard *card,
                               const char   *password);

/* don't under any circumstances call these functions */
#ifdef USD_SMARTCARD_ENABLE_INTERNAL_API

UsdSmartcard *_usd_smartcard_new (SECMODModule *module,
                                  CK_SLOT_ID    slot_id,
                                  gint          slot_series);
UsdSmartcard *_usd_smartcard_new_from_name (SECMODModule *module,
                                            const char   *name);

void _usd_smartcard_set_state (UsdSmartcard      *card,
                               UsdSmartcardState  state);
#endif

#ifdef __cplusplus
}
#endif
#endif                                /* USD_SMARTCARD_H */
