/* usd-smartcard.c - smartcard object
 *
 * Copyright (C) 2006 Ray Strode <rstrode@redhat.com>
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
#define USD_SMARTCARD_ENABLE_INTERNAL_API
#include "usd-smartcard.h"

#include <errno.h>
#include <string.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <cert.h>
#include <nss.h>
#include <pk11func.h>
#include <prerror.h>
#include <secmod.h>
#include <secerr.h>

struct _UsdSmartcardPrivate {
        SECMODModule *module;
        UsdSmartcardState state;

        CK_SLOT_ID slot_id;
        int slot_series;

        PK11SlotInfo *slot;
        char *name;

        CERTCertificate *signing_certificate;
        CERTCertificate *encryption_certificate;
};

static void usd_smartcard_finalize (GObject *object);
static void usd_smartcard_class_install_signals (UsdSmartcardClass *card_class);
static void usd_smartcard_class_install_properties (UsdSmartcardClass *card_class);
static void usd_smartcard_set_property (GObject       *object,
                                        guint          prop_id,
                                        const GValue  *value,
                                        GParamSpec    *pspec);
static void usd_smartcard_get_property (GObject     *object,
                                        guint        prop_id,
                                        GValue      *value,
                                        GParamSpec  *pspec);
static void usd_smartcard_set_name (UsdSmartcard *card, const char *name);
static void usd_smartcard_set_slot_id (UsdSmartcard *card,
                                       int                 slot_id);
static void usd_smartcard_set_slot_series (UsdSmartcard *card,
                                           int          slot_series);
static void usd_smartcard_set_module (UsdSmartcard *card,
                                      SECMODModule *module);

static PK11SlotInfo *usd_smartcard_find_slot_from_id (UsdSmartcard *card,
                                                      int slot_id);

static PK11SlotInfo *usd_smartcard_find_slot_from_card_name (UsdSmartcard *card,
                                                             const char  *card_name);
#ifndef USD_SMARTCARD_DEFAULT_SLOT_ID
#define USD_SMARTCARD_DEFAULT_SLOT_ID ((gulong) -1)
#endif

#ifndef USD_SMARTCARD_DEFAULT_SLOT_SERIES
#define USD_SMARTCARD_DEFAULT_SLOT_SERIES -1
#endif

enum {
        PROP_0 = 0,
        PROP_NAME,
        PROP_SLOT_ID,
        PROP_SLOT_SERIES,
        PROP_MODULE,
        NUMBER_OF_PROPERTIES
};

enum {
        INSERTED,
        REMOVED,
        NUMBER_OF_SIGNALS
};

static guint usd_smartcard_signals[NUMBER_OF_SIGNALS] = { 0 };

G_DEFINE_TYPE (UsdSmartcard, usd_smartcard, G_TYPE_OBJECT);

static void
usd_smartcard_class_init (UsdSmartcardClass *card_class)
{
        GObjectClass *gobject_class;

        gobject_class = G_OBJECT_CLASS (card_class);

        gobject_class->finalize = usd_smartcard_finalize;

        usd_smartcard_class_install_signals (card_class);
        usd_smartcard_class_install_properties (card_class);

        g_type_class_add_private (card_class,
                                  sizeof (UsdSmartcardPrivate));
}

static void
usd_smartcard_class_install_signals (UsdSmartcardClass *card_class)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (card_class);

        usd_smartcard_signals[INSERTED] =
                g_signal_new ("inserted",
                          G_OBJECT_CLASS_TYPE (object_class),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (UsdSmartcardClass,
                                           inserted),
                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);

        usd_smartcard_signals[REMOVED] =
                g_signal_new ("removed",
                          G_OBJECT_CLASS_TYPE (object_class),
                          G_SIGNAL_RUN_LAST,
                          G_STRUCT_OFFSET (UsdSmartcardClass,
                                           removed),
                          NULL, NULL, g_cclosure_marshal_VOID__VOID,
                          G_TYPE_NONE, 0);
}

static void
usd_smartcard_class_install_properties (UsdSmartcardClass *card_class)
{
        GObjectClass *object_class;
        GParamSpec *param_spec;

        object_class = G_OBJECT_CLASS (card_class);
        object_class->set_property = usd_smartcard_set_property;
        object_class->get_property = usd_smartcard_get_property;

        param_spec = g_param_spec_ulong ("slot-id", _("Slot ID"),
                                   _("The slot the card is in"),
                                   1, G_MAXULONG,
                                   USD_SMARTCARD_DEFAULT_SLOT_ID,
                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_SLOT_ID, param_spec);

        param_spec = g_param_spec_int ("slot-series", _("Slot Series"),
                                   _("per-slot card identifier"),
                                   -1, G_MAXINT,
                                   USD_SMARTCARD_DEFAULT_SLOT_SERIES,
                                   G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_SLOT_SERIES, param_spec);

        param_spec = g_param_spec_string ("name", _("name"),
                                      _("name"), NULL,
                                      G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_NAME, param_spec);

        param_spec = g_param_spec_pointer ("module", _("Module"),
                                       _("smartcard driver"),
                                       G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_MODULE, param_spec);
}

static void
usd_smartcard_set_property (GObject       *object,
                            guint          prop_id,
                            const GValue  *value,
                            GParamSpec    *pspec)
{
        UsdSmartcard *card = USD_SMARTCARD (object);

        switch (prop_id) {
                case PROP_NAME:
                        usd_smartcard_set_name (card, g_value_get_string (value));
                        break;

                case PROP_SLOT_ID:
                        usd_smartcard_set_slot_id (card,
                                                   g_value_get_ulong (value));
                        break;

                case PROP_SLOT_SERIES:
                        usd_smartcard_set_slot_series (card,
                                                       g_value_get_int (value));
                        break;

                case PROP_MODULE:
                        usd_smartcard_set_module (card,
                                                  (SECMODModule *)
                                                  g_value_get_pointer (value));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

CK_SLOT_ID
usd_smartcard_get_slot_id (UsdSmartcard *card)
{
        return card->priv->slot_id;
}

UsdSmartcardState
usd_smartcard_get_state (UsdSmartcard *card)
{
        return card->priv->state;
}

char *
usd_smartcard_get_name (UsdSmartcard *card)
{
        return g_strdup (card->priv->name);
}

gboolean
usd_smartcard_is_login_card (UsdSmartcard *card)
{
        const char *login_card_name;
        login_card_name = g_getenv ("PKCS11_LOGIN_TOKEN_NAME");

        if ((login_card_name == NULL) || (card->priv->name == NULL)) {
                return FALSE;
        }

        if (strcmp (card->priv->name, login_card_name) == 0) {
                return TRUE;
        }

        return FALSE;
}

static void
usd_smartcard_get_property (GObject    *object,
                            guint        prop_id,
                            GValue      *value,
                            GParamSpec  *pspec)
{
        UsdSmartcard *card = USD_SMARTCARD (object);

        switch (prop_id) {
                case PROP_NAME:
                        g_value_take_string (value,
                                             usd_smartcard_get_name (card));
                        break;

                case PROP_SLOT_ID:
                        g_value_set_ulong (value,
                                           (gulong) usd_smartcard_get_slot_id (card));
                        break;

                case PROP_SLOT_SERIES:
                        g_value_set_int (value,
                                         usd_smartcard_get_slot_series (card));
                        break;

                default:
                        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
}

static void
usd_smartcard_set_name (UsdSmartcard *card,
                        const char   *name)
{
        if (name == NULL) {
                return;
        }

        if ((card->priv->name == NULL) ||
            (strcmp (card->priv->name, name) != 0)) {
                g_free (card->priv->name);
                card->priv->name = g_strdup (name);

                if (card->priv->slot == NULL) {
                        card->priv->slot = usd_smartcard_find_slot_from_card_name (card,
                                                                                     card->priv->name);

                        if (card->priv->slot != NULL) {
                                int slot_id, slot_series;

                                slot_id = PK11_GetSlotID (card->priv->slot);
                                if (slot_id != card->priv->slot_id) {
                                        usd_smartcard_set_slot_id (card, slot_id);
                                }

                                slot_series = PK11_GetSlotSeries (card->priv->slot);
                                if (slot_series != card->priv->slot_series) {
                                        usd_smartcard_set_slot_series (card, slot_series);
                                }

                                _usd_smartcard_set_state (card, USD_SMARTCARD_STATE_INSERTED);
                        } else {
                                _usd_smartcard_set_state (card, USD_SMARTCARD_STATE_REMOVED);
                        }
                }

                g_object_notify (G_OBJECT (card), "name");
        }
}

static void
usd_smartcard_set_slot_id (UsdSmartcard *card,
                           int           slot_id)
{
        if (card->priv->slot_id != slot_id) {
                card->priv->slot_id = slot_id;

                if (card->priv->slot == NULL) {
                        card->priv->slot = usd_smartcard_find_slot_from_id (card,
                                                                             card->priv->slot_id);

                        if (card->priv->slot != NULL) {
                                const char *card_name;

                                card_name = PK11_GetTokenName (card->priv->slot);
                                if ((card->priv->name == NULL) ||
                                    ((card_name != NULL) &&
                                    (strcmp (card_name, card->priv->name) != 0))) {
                                        usd_smartcard_set_name (card, card_name);
                                }

                                _usd_smartcard_set_state (card, USD_SMARTCARD_STATE_INSERTED);
                        } else {
                                _usd_smartcard_set_state (card, USD_SMARTCARD_STATE_REMOVED);
                        }
                }

                g_object_notify (G_OBJECT (card), "slot-id");
        }
}

static void
usd_smartcard_set_slot_series (UsdSmartcard *card,
                               int           slot_series)
{
        if (card->priv->slot_series != slot_series) {
                card->priv->slot_series = slot_series;
                g_object_notify (G_OBJECT (card), "slot-series");
        }
}

static void
usd_smartcard_set_module (UsdSmartcard *card,
                          SECMODModule *module)
{
        gboolean should_notify;

        if (card->priv->module != module) {
                should_notify = TRUE;
        } else {
                should_notify = FALSE;
        }

        if (card->priv->module != NULL) {
                SECMOD_DestroyModule (card->priv->module);
                card->priv->module = NULL;
        }

        if (module != NULL) {
                card->priv->module = SECMOD_ReferenceModule (module);
        }

        if (should_notify) {
                g_object_notify (G_OBJECT (card), "module");
        }
}

int
usd_smartcard_get_slot_series (UsdSmartcard *card)
{
        return card->priv->slot_series;
}

static void
usd_smartcard_init (UsdSmartcard *card)
{

        g_debug ("initializing smartcard ");

        card->priv = G_TYPE_INSTANCE_GET_PRIVATE (card,
                                                  USD_TYPE_SMARTCARD,
                                                  UsdSmartcardPrivate);

        if (card->priv->slot != NULL) {
                card->priv->name = g_strdup (PK11_GetTokenName (card->priv->slot));
        }
}

static void usd_smartcard_finalize (GObject *object)
{
        UsdSmartcard *card;
        GObjectClass *gobject_class;

        card = USD_SMARTCARD (object);

        g_free (card->priv->name);

        usd_smartcard_set_module (card, NULL);

        gobject_class = G_OBJECT_CLASS (usd_smartcard_parent_class);

        gobject_class->finalize (object);
}

GQuark usd_smartcard_error_quark (void)
{
        static GQuark error_quark = 0;

        if (error_quark == 0) {
                error_quark = g_quark_from_static_string ("usd-smartcard-error-quark");
        }

        return error_quark;
}

UsdSmartcard *
_usd_smartcard_new (SECMODModule *module,
                    CK_SLOT_ID    slot_id,
                    int           slot_series)
{
        UsdSmartcard *card;

        g_return_val_if_fail (module != NULL, NULL);
        g_return_val_if_fail (slot_id >= 1, NULL);
        g_return_val_if_fail (slot_series > 0, NULL);
        g_return_val_if_fail (sizeof (gulong) == sizeof (slot_id), NULL);

        card = USD_SMARTCARD (g_object_new (USD_TYPE_SMARTCARD,
                                             "module", module,
                                             "slot-id", (gulong) slot_id,
                                             "slot-series", slot_series,
                                             NULL));
        return card;
}

UsdSmartcard *
_usd_smartcard_new_from_name (SECMODModule *module,
                              const char   *name)
{
        UsdSmartcard *card;

        g_return_val_if_fail (module != NULL, NULL);
        g_return_val_if_fail (name != NULL, NULL);

        card = USD_SMARTCARD (g_object_new (USD_TYPE_SMARTCARD,
                                            "module", module,
                                            "name", name,
                                            NULL));
        return card;
}

void
_usd_smartcard_set_state (UsdSmartcard      *card,
                          UsdSmartcardState  state)
{
        if (card->priv->state != state) {
                card->priv->state = state;

                if (state == USD_SMARTCARD_STATE_INSERTED) {
                        g_signal_emit (card, usd_smartcard_signals[INSERTED], 0);
                } else if (state == USD_SMARTCARD_STATE_REMOVED) {
                        g_signal_emit (card, usd_smartcard_signals[REMOVED], 0);
                } else {
                        g_assert_not_reached ();
                }
        }
}

/* So we could conceivably make the closure data a pointer to the card
 * or something similiar and then emit signals when we want passwords,
 * but it's probably easier to just get the password up front and use
 * it.  So we just take the passed in g_malloc'd (well probably, who knows)
 * and strdup it using NSPR's memory allocation routines.
 */
static char *
usd_smartcard_password_handler (PK11SlotInfo *slot,
                                PRBool        is_retrying,
                                const char   *password)
{
        if (is_retrying) {
                return NULL;
        }

        return password != NULL? PL_strdup (password): NULL;
}

gboolean
usd_smartcard_unlock (UsdSmartcard *card,
                      const char   *password)
{
        SECStatus status;

        PK11_SetPasswordFunc ((PK11PasswordFunc) usd_smartcard_password_handler);

        /* we pass PR_TRUE to load certificates
        */
        status = PK11_Authenticate (card->priv->slot, PR_TRUE, (gpointer) password);

        if (status != SECSuccess) {
                g_debug ("could not unlock card - %d", status);
                return FALSE;
        }
        return TRUE;
}

static PK11SlotInfo *
usd_smartcard_find_slot_from_card_name (UsdSmartcard *card,
                                        const char   *card_name)
{
        int i;

        for (i = 0; i < card->priv->module->slotCount; i++) {
                const char *slot_card_name;

                slot_card_name = PK11_GetTokenName (card->priv->module->slots[i]);

                if ((slot_card_name != NULL) &&
                    (strcmp (slot_card_name, card_name) == 0)) {
                        return card->priv->module->slots[i];
                }
        }

        return NULL;
}

static PK11SlotInfo *
usd_smartcard_find_slot_from_id (UsdSmartcard *card,
                                 int           slot_id)
{
        int i;

        for (i = 0; i < card->priv->module->slotCount; i++) {
                if (PK11_GetSlotID (card->priv->module->slots[i]) == slot_id) {
                        return card->priv->module->slots[i];
                }
        }

        return NULL;
}
