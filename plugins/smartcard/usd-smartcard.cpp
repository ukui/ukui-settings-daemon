#include "usd-smartcard.h"

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


Smartcard * Smartcard::mSmartcard = nullptr;

Smartcard::Smartcard()
{
    this->usd_smartcard_class_install_properties();
    name = g_strdup (PK11_GetTokenName (this->slot));

}

Smartcard::~Smartcard()
{
    if(mSmartcard)
        delete mSmartcard;
    this->usd_smartcard_set_module(NULL);
}

Smartcard *Smartcard::SmartcardNew()
{
    if(nullptr == mSmartcard)
        mSmartcard = new Smartcard;
    return mSmartcard;
}

void Smartcard::usd_smartcard_class_install_properties()
{
    this->usd_smartcard_set_property();
    this->usd_smartcard_get_property();
}

void Smartcard::usd_smartcard_set_property()
{
    this->usd_smartcard_set_name(this->name);
    this->usd_smartcard_set_slot_id(this->slot_id);
    this->usd_smartcard_set_slot_series(this->slot_series);
    this->usd_smartcard_set_module(this->module);
}
void Smartcard::usd_smartcard_get_property()
{
    usd_smartcard_get_name(this);
    usd_smartcard_get_slot_id(this);
    usd_smartcard_get_slot_series(this);

}

PK11SlotInfo *
usd_smartcard_find_slot_from_card_name (Smartcard *card,
                                        const char   *card_name)
{
        int i;

        for (i = 0; i < card->module->slotCount; i++) {
                const char *slot_card_name;

                slot_card_name = PK11_GetTokenName (card->module->slots[i]);

                if ((slot_card_name != NULL) &&
                    (strcmp (slot_card_name, card_name) == 0)) {
                        return card->module->slots[i];
                }
        }

        return NULL;
}

void Smartcard::usd_smartcard_set_name(const char *name)
{
    if (name == NULL) {
            return;
    }

    if ((this->name == NULL) ||
        (strcmp (this->name, name) != 0)) {
        g_free (this->name);
        this->name = g_strdup (name);

        if (this->slot == NULL) {
            this->slot = usd_smartcard_find_slot_from_card_name (this, this->name);

            if (this->slot != NULL) {
                int slot_id, slot_series;

                slot_id = PK11_GetSlotID (this->slot);
                if (slot_id != this->slot_id) {
                        usd_smartcard_set_slot_id (slot_id);
                }

                slot_series = PK11_GetSlotSeries (this->slot);
                if (slot_series != this->slot_series) {
                        usd_smartcard_set_slot_series (slot_series);
                }

                _usd_smartcard_set_state (this, USD_SMARTCARD_STATE_INSERTED);
            } else {
                _usd_smartcard_set_state (this, USD_SMARTCARD_STATE_REMOVED);
            }
        }

        g_object_notify (G_OBJECT (this), "name");
    }
}

char * Smartcard::usd_smartcard_get_name(Smartcard *card)
{
    return g_strdup (card->name);
}

gboolean Smartcard::usd_smartcard_is_login_card(Smartcard *card)
{
    const char *login_card_name;
    login_card_name = getenv("PKCS11_LOGIN_TOKEN_NAME");
    if ((login_card_name == NULL) || (card->name == NULL)) {
        return FALSE;
    }

    if (strcmp (card->name, login_card_name) == 0) {
        return TRUE;
    }

    return FALSE;
}

void _usd_smartcard_set_state (Smartcard      *card,
                               SmartcardState  state)
{
        if (card->state != state) {
                card->state = state;

                if (state == USD_SMARTCARD_STATE_INSERTED) {
                        //g_signal_emit (card, usd_smartcard_signals[INSERTED], 0);
                } else if (state == USD_SMARTCARD_STATE_REMOVED) {
                        //g_signal_emit (card, usd_smartcard_signals[REMOVED], 0);
                } else {
                        g_assert_not_reached ();
                }
        }
}

PK11SlotInfo *
usd_smartcard_find_slot_from_id (Smartcard *card,
                                 int      slot_id)
{
        int i;

        for (i = 0; i < card->module->slotCount; i++) {
                if (PK11_GetSlotID (card->module->slots[i]) == slot_id) {
                        return card->module->slots[i];
                }
        }
        return NULL;
}

void Smartcard::usd_smartcard_set_slot_id(int slot_id)
{
    if (this->slot_id != slot_id) {
        this->slot_id = slot_id;

        if (this->slot == NULL) {
            this->slot = usd_smartcard_find_slot_from_id (this,this->slot_id);

            if (this->slot != NULL) {
                const char *card_name;

                card_name = PK11_GetTokenName (this->slot);
                if ((this->name == NULL) ||
                    ((card_name != NULL) &&
                    (strcmp (card_name, this->name) != 0))) {
                        usd_smartcard_set_name (card_name);
                }

                _usd_smartcard_set_state (this,USD_SMARTCARD_STATE_INSERTED);
            } else {
                _usd_smartcard_set_state (this,USD_SMARTCARD_STATE_REMOVED);
            }
        }

        g_object_notify (G_OBJECT (this), "slot-id");
    }
}

CK_SLOT_ID Smartcard::usd_smartcard_get_slot_id (Smartcard *card)
{
        return card->slot_id;
}

void Smartcard::usd_smartcard_set_slot_series(int slot_series)
{
    if (this->slot_series != slot_series) {
                    this->slot_series = slot_series;
                    g_object_notify (G_OBJECT (this), "slot-series");
            }
}

gint Smartcard::usd_smartcard_get_slot_series(Smartcard *card)
{
    return card->slot_series;
}

void Smartcard::usd_smartcard_set_module (SECMODModule *module)
{
        gboolean should_notify;

        if (this->module != module) {
                should_notify = TRUE;
        } else {
                should_notify = FALSE;
        }

        if (this->module != NULL) {
                SECMOD_DestroyModule (this->module);
                this->module = NULL;
        }

        if (module != NULL) {
                this->module = SECMOD_ReferenceModule (module);
        }

        if (should_notify) {
                g_object_notify (G_OBJECT (this), "module");
        }
}

Smartcard *Smartcard::_usd_smartcard_new_from_name(SECMODModule *module, const char *name)
{
    Smartcard *card;
    g_return_val_if_fail (module != NULL, NULL);
    g_return_val_if_fail (name != NULL, NULL);

    card = this->SmartcardNew();
    this->module = module;
    this->name =(char *) name;

    return card;
}

Smartcard *Smartcard::_usd_smartcard_new(SECMODModule *module, CK_SLOT_ID slot_id, gint slot_series)
{
    Smartcard *card;

    g_return_val_if_fail (module != NULL, NULL);
    g_return_val_if_fail (slot_id >= 1, NULL);
    g_return_val_if_fail (slot_series > 0, NULL);
    g_return_val_if_fail (sizeof (gulong) == sizeof (slot_id), NULL);

    card = this->SmartcardNew();
    card->module = module;
    card->slot_id = slot_id;
    card->slot_series = slot_series;

    return card;
}
