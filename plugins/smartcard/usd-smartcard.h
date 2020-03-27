#ifndef SMARTCARD_H
#define SMARTCARD_H

#include "smartcard-manager.h"
#include <QObject>

enum SmartcardError {
    USD_SMARTCARD_ERROR_GENERIC = 0,
};

enum SmartcardState {
    USD_SMARTCARD_STATE_INSERTED = 0,
    USD_SMARTCARD_STATE_REMOVED,
};


class Smartcard : public QObject
{
    Q_OBJECT
public:
    ~Smartcard();
    static Smartcard *SmartcardNew();
    void usd_smartcard_class_install_properties();
    void usd_smartcard_set_property();
    void usd_smartcard_get_property();

    void usd_smartcard_set_name(const char *name);
    char *usd_smartcard_get_name(Smartcard *card);
    void usd_smartcard_set_slot_id(int   slot_id);
    CK_SLOT_ID usd_smartcard_get_slot_id(Smartcard *card);
    void usd_smartcard_set_slot_series(int   slot_series);
    int usd_smartcard_get_slot_series(Smartcard *card);
    void usd_smartcard_set_module(SECMODModule *module);

    Smartcard *_usd_smartcard_new_from_name (SECMODModule *module,
                                                const char   *name);
    Smartcard *_usd_smartcard_new (SECMODModule *module,
                                   CK_SLOT_ID    slot_id,
                                   gint          slot_series);
    gboolean usd_smartcard_is_login_card (Smartcard *card);

private:
    Smartcard();
    Smartcard(Smartcard&)=delete;
    Smartcard&operator = (const Smartcard&)=delete;

    friend void _usd_smartcard_set_state (Smartcard      *card,
                                          SmartcardState  state);
    friend PK11SlotInfo * usd_smartcard_find_slot_from_id (Smartcard *card,
                                                        int      slot_id);
    friend PK11SlotInfo * usd_smartcard_find_slot_from_card_name (Smartcard *card,
                                                           const char   *card_name);


private:
    static Smartcard *mSmartcard;

    SECMODModule *module;
    SmartcardState state;

    CK_SLOT_ID slot_id;
    int slot_series;

    PK11SlotInfo *slot;
    char *name;

    CERTCertificate *signing_certificate;
    CERTCertificate *encryption_certificate;
};

#endif // SMARTCARD_H
