#include "smartcard-manager.h"
#include "clib-syslog.h"
#include "usd-smartcard.h"
#include <QDebug>

#ifndef USD_SMARTCARD_MANAGER_NSS_DB
#define USD_SMARTCARD_MANAGER_NSS_DB "/usr/local/etc//pki/nssdb"
#endif

enum {
        PROP_0 = 0,
        PROP_MODULE_PATH,
        NUMBER_OF_PROPERTIES
};

enum {
        SMARTCARD_INSERTED = 0,
        SMARTCARD_REMOVED,
        ERROR,
        NUMBER_OF_SIGNALS
};

//static guint usd_smartcard_manager_signals[NUMBER_OF_SIGNALS] = { 0 };
SmartcardManager *SmartcardManager::mManager = nullptr;

SmartcardManager::SmartcardManager(QObject *parent)
{
    poll_timeout_id = 0;
    is_unstoppable = FALSE;
    module = NULL;
    smartcards = g_hash_table_new_full (g_str_hash,
                                        g_str_equal,
                                        (GDestroyNotify) g_free,
                                        (GDestroyNotify) g_object_unref);

    this->usd_smartcard_manager_class_install_properties();
}

SmartcardManager::~SmartcardManager()
{
    usd_smartcard_manager_stop_now(this);
    g_hash_table_destroy(smartcards);
    smartcards = NULL;
    if(mManager)
        delete mManager;
}

SmartcardManager *SmartcardManager::managerNew()
{
    if(nullptr == mManager)
        mManager = new SmartcardManager(nullptr);
    return mManager;
}

void SmartcardManager::usd_smartcard_manager_class_install_properties()
{
    this->usd_smartcard_manager_set_property();
    this->usd_smartcard_manager_get_property();
}

void SmartcardManager::usd_smartcard_manager_card_inserted_handler(Smartcard   *card)
{
    _usd_smartcard_set_state (card, USD_SMARTCARD_STATE_INSERTED);
}

void SmartcardManager::usd_smartcard_manager_card_removed_handler(Smartcard   *card)
{
    _usd_smartcard_set_state (card, USD_SMARTCARD_STATE_REMOVED);
}

void SmartcardManager::usd_smartcard_manager_set_property()
{
    if ((this->module_path == NULL) && (module_path == NULL)) {
            return;
    }

    if (((this->module_path == NULL) ||
     (module_path == NULL) ||
     (strcmp (this->module_path, module_path) != 0))) {
            g_free (this->module_path);
            this->module_path = g_strdup (module_path);
            g_object_notify (G_OBJECT (this), "module-path");
    }
}

void SmartcardManager::usd_smartcard_manager_get_property()
{
     /*char *module_path;
     module_path = this->module_path;
     g_value_set_string (this, module_path);
     g_free (module_path);*/
}
char *SmartcardManager::usd_smartcard_manager_get_module_path()
{
    return this->module_path;
}


static gboolean load_nss (GError **error)
{
    SECStatus status = SECSuccess;
    static const guint32 flags =
    NSS_INIT_READONLY | NSS_INIT_NOCERTDB | NSS_INIT_NOMODDB |
    NSS_INIT_FORCEOPEN | NSS_INIT_NOROOTINIT |
    NSS_INIT_OPTIMIZESPACE | NSS_INIT_PK11RELOAD;
	syslog(LOG_ERR,"---LOAD_NSS----");
    g_debug ("attempting to load NSS database '%s'",
             USD_SMARTCARD_MANAGER_NSS_DB);

    status = NSS_Initialize (USD_SMARTCARD_MANAGER_NSS_DB,
                            "", "", SECMOD_DB, flags);
	syslog(LOG_ERR,"-----------1 ---------");
    if (status != SECSuccess) {
           gsize error_message_size;
           char *error_message;

           error_message_size = PR_GetErrorTextLength ();

           if (error_message_size == 0) {
                   g_debug ("NSS security system could not be initialized");
                   /*g_set_error (error,
                                USD_SMARTCARD_MANAGER_ERROR,
                                USD_SMARTCARD_MANAGER_ERROR_WITH_NSS,
                                _("NSS security system could not be initialized"));*/
                   goto out;
           }

           error_message = (char *)g_slice_alloc0 (error_message_size);
           PR_GetErrorText (error_message);

           /*g_set_error (error,
                        USD_SMARTCARD_MANAGER_ERROR,
                        USD_SMARTCARD_MANAGER_ERROR_WITH_NSS,
                        "%s", error_message);*/
           g_debug ("NSS security system could not be initialized - %s",
                     error_message);

           g_slice_free1 (error_message_size, error_message);

           goto out;
    }

    g_debug ("NSS database successfully loaded");
    return TRUE;

out:
    g_debug ("NSS database couldn't be successfully loaded");
    return FALSE;
}

static SECMODModule * load_driver (char    *module_path,
             GError **error)  //加载驱动程序
{
        SECMODModule *module;
        char *module_spec;
        gboolean module_explicitly_specified;

        g_debug ("attempting to load driver...");

        module = NULL;
        module_explicitly_specified = module_path != NULL;
        if (module_explicitly_specified) {
                module_spec = g_strdup_printf ("library=\"%s\"", module_path);
                g_debug ("loading smartcard driver using spec '%s'",
                          module_spec);

                module = SECMOD_LoadUserModule (module_spec,
                                                NULL /* parent */,
                                                FALSE /* recurse */);
                g_free (module_spec);
                module_spec = NULL;

        } else {
                SECMODModuleList *modules, *tmp;

                modules = SECMOD_GetDefaultModuleList ();

                for (tmp = modules; tmp != NULL; tmp = tmp->next) {
                        if (!SECMOD_HasRemovableSlots (tmp->module) ||
                            !tmp->module->loaded)
                                continue;

                        module = SECMOD_ReferenceModule (tmp->module);
                        break;
                }
        }

        if (!module_explicitly_specified && module == NULL) {
                /*g_set_error (error,
                             USD_SMARTCARD_MANAGER_ERROR,
                             USD_SMARTCARD_MANAGER_ERROR_LOADING_DRIVER,
                             _("no suitable smartcard driver could be found"));*/
        } else if (module == NULL || !module->loaded) {

                gsize error_message_size;
                char *error_message;

                if (module != NULL && !module->loaded) {
                        g_debug ("module found but not loaded?!");
                        SECMOD_DestroyModule (module);
                        module = NULL;
                }

                error_message_size = PR_GetErrorTextLength ();

                if (error_message_size == 0) {
                        g_debug ("smartcard driver '%s' could not be loaded",
                                  module_path);
                        /*g_set_error (error,
                                     USD_SMARTCARD_MANAGER_ERROR,
                                     USD_SMARTCARD_MANAGER_ERROR_LOADING_DRIVER,
                                     _("smartcard driver '%s' could not be "
                                       "loaded"), module_path);*/
                        goto out;
                }

                error_message = (char *)g_slice_alloc0 (error_message_size);
                PR_GetErrorText (error_message);

                /*g_set_error (error,
                             USD_SMARTCARD_MANAGER_ERROR,
                             USD_SMARTCARD_MANAGER_ERROR_LOADING_DRIVER,
                             "%s", error_message);*/

                g_debug ("smartcard driver '%s' could not be loaded - %s",
                          module_path, error_message);
                g_slice_free1 (error_message_size, error_message);
        }

out:
        return module;
}

static gboolean open_pipe (int *write_fd,
                           int *read_fd)
{
        int pipe_fds[2] = { -1, -1 };

        g_assert (write_fd != NULL);
        g_assert (read_fd != NULL);

        if (pipe (pipe_fds) < 0) {
                return FALSE;
        }

        if (fcntl (pipe_fds[0], F_SETFD, FD_CLOEXEC) < 0) {
                close (pipe_fds[0]);
                close (pipe_fds[1]);
                return FALSE;
        }

        if (fcntl (pipe_fds[1], F_SETFD, FD_CLOEXEC) < 0) {
                close (pipe_fds[0]);
                close (pipe_fds[1]);
                return FALSE;
        }

        *read_fd = pipe_fds[0];
        *write_fd = pipe_fds[1];

        return TRUE;
}

static gboolean slot_id_hash (CK_SLOT_ID *slot_id)
{
        guint32 upper_bits, lower_bits;
        int temp;

        if (sizeof (CK_SLOT_ID) == sizeof (int)) {
                return g_int_hash (slot_id);
        }

        upper_bits = ((*slot_id) >> 31) - 1;
        lower_bits = (*slot_id) & 0xffffffff;

        /* The upper bits are almost certainly always zero,
         * so let's degenerate to g_int_hash for the
         * (very) common case
         */
        temp = lower_bits + upper_bits;
        return upper_bits + g_int_hash (&temp);
}

static gboolean
slot_id_equal (CK_SLOT_ID *slot_id_1,
               CK_SLOT_ID *slot_id_2)
{
        g_assert (slot_id_1 != NULL);
        g_assert (slot_id_2 != NULL);

        return *slot_id_1 == *slot_id_2;
}

SmartcardManagerWorker *
usd_smartcard_manager_worker_new (int write_fd)
{
        SmartcardManagerWorker *worker;

        worker = g_slice_new0 (SmartcardManagerWorker);
        worker->write_fd = write_fd;
        worker->module = NULL;

        worker->smartcards =
                g_hash_table_new_full ((GHashFunc) slot_id_hash,
                                       (GEqualFunc) slot_id_equal,
                                       (GDestroyNotify) g_free,
                                       (GDestroyNotify) g_object_unref);

        return worker;
}

static gboolean
read_bytes (int      fd,
            gpointer bytes,
            gsize    num_bytes)
{
        size_t bytes_left;
        size_t total_bytes_read;
        ssize_t bytes_read;

        bytes_left = (size_t) num_bytes;
        total_bytes_read = 0;

        do {
                bytes_read = read (fd,
                                   (char *) bytes + total_bytes_read,
                                   bytes_left);
                g_assert (bytes_read <= (ssize_t) bytes_left);

                if (bytes_read <= 0) {
                        if ((bytes_read < 0) && (errno == EINTR || errno == EAGAIN)) {
                                continue;
                        }

                        bytes_left = 0;
                } else {
                        bytes_left -= bytes_read;
                        total_bytes_read += bytes_read;
                }
        } while (bytes_left > 0);

        if (total_bytes_read <  (size_t) num_bytes) {
                return FALSE;
        }

        return TRUE;
}

static gboolean
write_bytes (int           fd,
             gconstpointer bytes,
             gsize         num_bytes)
{
        size_t bytes_left;
        size_t total_bytes_written;
        ssize_t bytes_written;

        bytes_left = (size_t) num_bytes;
        total_bytes_written = 0;

        do {
                bytes_written = write (fd,
                                       (char *) bytes + total_bytes_written,
                                       bytes_left);
                g_assert (bytes_written <= (ssize_t) bytes_left);

                if (bytes_written <= 0) {
                        if ((bytes_written < 0) && (errno == EINTR || errno == EAGAIN)) {
                                continue;
                        }

                        bytes_left = 0;
                } else {
                        bytes_left -= bytes_written;
                        total_bytes_written += bytes_written;
                }
        } while (bytes_left > 0);

        if (total_bytes_written <  (size_t) num_bytes) {
                return FALSE;
        }

        return TRUE;
}

static gboolean
write_smartcard (int           fd,
                 Smartcard *card)
{
        gsize card_name_size;
        char *card_name;

        card_name = card->usd_smartcard_get_name (card);
        card_name_size = strlen (card_name) + 1;

        if (!write_bytes (fd, &card_name_size, sizeof (card_name_size))) {
                g_free (card_name);
                return FALSE;
        }

        if (!write_bytes (fd, card_name, card_name_size)) {
                g_free (card_name);
                return FALSE;
        }
        g_free (card_name);

        return TRUE;
}

static Smartcard *
read_smartcard (int           fd,
                SECMODModule *module)
{
        Smartcard *card;
        char *card_name;
        gsize card_name_size;

        card_name_size = 0;
        if (!read_bytes (fd, &card_name_size, sizeof (card_name_size))) {
                return NULL;
        }

        card_name = (char *)g_slice_alloc0 (card_name_size);
        if (!read_bytes (fd, card_name, card_name_size)) {
                g_slice_free1 (card_name_size, card_name);
                return NULL;
        }
        card = card->_usd_smartcard_new_from_name (module, card_name);
        g_slice_free1 (card_name_size, card_name);

        return card;
}


static gboolean
usd_smartcard_manager_worker_emit_smartcard_removed (SmartcardManagerWorker  *worker,
                                                     Smartcard               *card,
                                                     GError                   **error)
{
        g_debug ("card '%s' removed!", card->usd_smartcard_get_name (card));

        if (!write_bytes (worker->write_fd, "R", 1)) {
                goto error_out;
        }

        if (!write_smartcard (worker->write_fd, card)) {
                goto error_out;
        }

        return TRUE;

error_out:
        /*g_set_error (error, USD_SMARTCARD_MANAGER_ERROR,
                     USD_SMARTCARD_MANAGER_ERROR_REPORTING_EVENTS,
                     "%s", g_strerror (errno));*/
        return FALSE;
}

static gboolean
usd_smartcard_manager_worker_emit_smartcard_inserted (SmartcardManagerWorker  *worker,
                                                      Smartcard               *card,
                                                      GError                    **error)
{
        g_debug ("card '%s' inserted!", card->usd_smartcard_get_name (card));
        if (!write_bytes (worker->write_fd, "I", 1)) {
                goto error_out;
        }

        if (!write_smartcard (worker->write_fd, card)) {
                goto error_out;
        }

        return TRUE;

error_out:
       /* g_set_error (error, USD_SMARTCARD_MANAGER_ERROR,
                     USD_SMARTCARD_MANAGER_ERROR_REPORTING_EVENTS,
                     "%s", g_strerror (errno));*/
        return FALSE;
}

static gboolean
usd_smartcard_manager_worker_watch_for_and_process_event (SmartcardManagerWorker  *worker,
                                                          GError                    **error)
{
        PK11SlotInfo *slot;
        CK_SLOT_ID slot_id, *key = NULL;
        int slot_series, card_slot_series;
        Smartcard *card;
        GError *processing_error;
        gboolean ret;

        g_debug ("waiting for card event");
        ret = FALSE;

        slot = SECMOD_WaitForAnyTokenEvent (worker->module, 0, PR_SecondsToInterval (1));
        processing_error = NULL;

        if (slot == NULL) {
                int error_code;

                error_code = PORT_GetError ();
                if ((error_code == 0) || (error_code == SEC_ERROR_NO_EVENT)) {
                        g_debug ("spurrious event occurred");
                        return TRUE;
                }

                /* FIXME: is there a function to convert from a PORT error
                 * code to a translated string?
                 */
                /*g_set_error (error, USD_SMARTCARD_MANAGER_ERROR,
                             USD_SMARTCARD_MANAGER_ERROR_WITH_NSS,
                             _("encountered unexpected error while "
                               "waiting for smartcard events"));*/
                goto out;
        }

        /* the slot id and series together uniquely identify a card.
         * You can never have two cards with the same slot id at the
         * same time, however (I think), so we can key off of it.
         */
        slot_id = PK11_GetSlotID (slot);
        slot_series = PK11_GetSlotSeries (slot);

        /* First check to see if there is a card that we're currently
         * tracking in the slot.
         */
        key = g_new (CK_SLOT_ID, 1);
        *key = slot_id;
        card =(Smartcard *)g_hash_table_lookup (worker->smartcards, key);

        if (card != NULL) {
                card_slot_series = card->usd_smartcard_get_slot_series (card);
        } else {
                card_slot_series = -1;
        }

        if (PK11_IsPresent (slot)) {
                /* Now, check to see if their is a new card in the slot.
                 * If there was a different card in the slot now than
                 * there was before, then we need to emit a removed signal
                 * for the old card (we don't want unpaired insertion events).
                 */
                if ((card != NULL) &&
                    card_slot_series != slot_series) {
                        if (!usd_smartcard_manager_worker_emit_smartcard_removed (worker, card, &processing_error)) {
                                g_propagate_error (error, processing_error);
                                goto out;
                        }
                }

                card = card->_usd_smartcard_new (worker->module,
                                           slot_id, slot_series);

                g_hash_table_replace (worker->smartcards,
                                      key, card);
                key = NULL;

                if (!usd_smartcard_manager_worker_emit_smartcard_inserted (worker,
                                                                           card,
                                                                           &processing_error)) {
                        g_propagate_error (error, processing_error);
                        goto out;
                }
        } else {
                /* if we aren't tracking the card, just discard the event.
                 * We don't want unpaired remove events.  Note on startup
                 * NSS will generate an "insertion" event if a card is
                 * already inserted in the slot.
                 */
                if ((card != NULL)) {
                        /* FIXME: i'm not sure about this code.  Maybe we
                         * shouldn't do this at all, or maybe we should do it
                         * n times (where n = slot_series - card_slot_series + 1)
                         *
                         * Right now, i'm just doing it once.
                         */
                        if ((slot_series - card_slot_series) > 1) {

                                if (!usd_smartcard_manager_worker_emit_smartcard_removed (worker, card, &processing_error)) {
                                        g_propagate_error (error, processing_error);
                                        goto out;
                                }
                                g_hash_table_remove (worker->smartcards, key);

                                card = card->_usd_smartcard_new (worker->module,
                                                                slot_id, slot_series);
                                g_hash_table_replace (worker->smartcards,
                                                      key, card);
                                key = NULL;
                                if (!usd_smartcard_manager_worker_emit_smartcard_inserted (worker, card, &processing_error)) {
                                        g_propagate_error (error, processing_error);
                                        goto out;
                                }
                        }

                        if (!usd_smartcard_manager_worker_emit_smartcard_removed (worker, card, &processing_error)) {
                                g_propagate_error (error, processing_error);
                                goto out;
                        }

                        g_hash_table_remove (worker->smartcards, key);
                        card = NULL;
                } else {
                        g_debug ("got spurious remove event");
                }
        }
        ret = TRUE;

out:
        g_free (key);
        PK11_FreeSlot (slot);

            return ret;
}

static void //员工卡释放
usd_smartcard_manager_worker_free (SmartcardManagerWorker *worker)
{
        if (worker->smartcards != NULL) {
                g_hash_table_destroy (worker->smartcards);
                worker->smartcards = NULL;
        }

        g_slice_free (SmartcardManagerWorker, worker);
}

static void
usd_smartcard_manager_worker_run (SmartcardManagerWorker *worker)
{
        GError *error;


        error = NULL;

        while (usd_smartcard_manager_worker_watch_for_and_process_event (worker, &error));

        if (error != NULL)  {
                g_debug ("could not process card event - %s", error->message);
                g_error_free (error);
        }

        usd_smartcard_manager_worker_free (worker);
}

gboolean usd_smartcard_manager_create_worker (SmartcardManager  *manager,
                                             int                  *worker_fd,
                                             GThread             **worker_thread)
{
        SmartcardManagerWorker *worker;
        int write_fd, read_fd;

        write_fd = -1;
        read_fd = -1;
        if (!open_pipe (&write_fd, &read_fd)) {
                return FALSE;
        }
        worker = usd_smartcard_manager_worker_new (write_fd);
        worker->module = manager->module;

        *worker_thread = g_thread_new ("UsdSmartcardManagerWorker", (GThreadFunc)
                                          usd_smartcard_manager_worker_run,
                                          worker);

        if (*worker_thread == NULL) {
                usd_smartcard_manager_worker_free (worker);
                return FALSE;
        }

        if (worker_fd) {
                *worker_fd = read_fd;
        }

        return TRUE;
}

void SmartcardManager::usd_smartcard_manager_emit_smartcard_inserted (SmartcardManager *manager,
                                                    Smartcard        *card)
{
        manager->is_unstoppable = TRUE;
        usd_smartcard_manager_card_inserted_handler(card);//smartcard_inserted();
        manager->is_unstoppable = FALSE;
}

void SmartcardManager::usd_smartcard_manager_emit_smartcard_removed (SmartcardManager *manager,
                                                    Smartcard        *card)
{
        manager->is_unstoppable = TRUE;
        usd_smartcard_manager_card_removed_handler(card);//smartcard_removed();
        manager->is_unstoppable = FALSE;
}

void usd_smartcard_manager_stop_watching_for_events (SmartcardManager  *manager)
{
        if (manager->smartcard_event_source != NULL) {
                g_source_destroy (manager->smartcard_event_source);
                manager->smartcard_event_source = NULL;
        }

        if (manager->worker_thread != NULL) {
                SECMOD_CancelWait (manager->module);
                manager->worker_thread = NULL;
        }
}

gboolean usd_smartcard_manager_stop_now (SmartcardManager *manager)
{
        if (manager->state == USD_SMARTCARD_MANAGER_STATE_STOPPED) {
                return FALSE;
        }

        manager->state = USD_SMARTCARD_MANAGER_STATE_STOPPED;
        usd_smartcard_manager_stop_watching_for_events (manager);

        if (manager->module != NULL) {
                SECMOD_DestroyModule (manager->module);
                manager->module = NULL;
        }

        if (manager->nss_is_loaded) {
                NSS_Shutdown ();
                manager->nss_is_loaded = FALSE;
        }

        g_debug ("smartcard manager stopped");

        return FALSE;
}

gboolean
usd_smartcard_manager_check_for_and_process_events (GIOChannel          *io_channel,
                                                    GIOCondition         condition,
                                                    SmartcardManager    *manager)
{
    Smartcard *card;
    gboolean should_stop;
    gchar event_type;
    char *card_name;
    int fd;

    card = NULL;
    should_stop = (condition & G_IO_HUP) || (condition & G_IO_ERR);

    if (should_stop) {
            g_debug ("received %s on event socket, stopping "
                      "manager...",
                      (condition & G_IO_HUP) && (condition & G_IO_ERR)?
                      "error and hangup" :
                      (condition & G_IO_HUP)?
                      "hangup" : "error");
    }

    if (!(condition & G_IO_IN)) {
            goto out;
    }

    fd = g_io_channel_unix_get_fd (io_channel);

    event_type = '\0';
    if (!read_bytes (fd, &event_type, 1)) {
            should_stop = TRUE;
            goto out;
    }

    card = read_smartcard (fd, manager->module);

    if (card == NULL) {
            should_stop = TRUE;
            goto out;
    }

    card_name = card->usd_smartcard_get_name (card);

    switch (event_type) {
            case 'I':
                    g_hash_table_replace (manager->smartcards,
                                          card_name, card);
                    card_name = NULL;

                    manager->usd_smartcard_manager_emit_smartcard_inserted (manager, card);
                    card = NULL;
                    break;

            case 'R':
                    manager->usd_smartcard_manager_emit_smartcard_removed (manager, card);
                    if (!g_hash_table_remove (manager->smartcards, card_name)) {
                            g_debug ("got removal event of unknown card!");
                    }
                    g_free (card_name);
                    card_name = NULL;
                    card = NULL;
                    break;

            default:
                    g_free (card_name);
                    card_name = NULL;
                    g_object_unref (card);

                    should_stop = TRUE;
                    break;
    }

out:
    if (should_stop) {
            /*GError *error;

            error = g_error_new (USD_SMARTCARD_MANAGER_ERROR,
                                 USD_SMARTCARD_MANAGER_ERROR_WATCHING_FOR_EVENTS,
                                 "%s", (condition & G_IO_IN) ? g_strerror (errno) : _("received error or hang up from event source"));

            usd_smartcard_manager_emit_error (manager, error);
            g_error_free (error);*/
            manager->is_unstoppable = FALSE;
            usd_smartcard_manager_stop_now (manager);
            return FALSE;
    }

    return TRUE;
}

void usd_smartcard_manager_event_processing_stopped_handler (SmartcardManager *manager)
{
        manager->smartcard_event_source = NULL;
        usd_smartcard_manager_stop_now (manager);
}

void usd_smartcard_manager_get_all_cards (SmartcardManager *manager)
{
        int i;

        for (i = 0; i < manager->module->slotCount; i++) {
                Smartcard *card;
                CK_SLOT_ID    slot_id;
                int          slot_series;
                char *card_name;

                slot_id = PK11_GetSlotID (manager->module->slots[i]);
                slot_series = PK11_GetSlotSeries (manager->module->slots[i]);

                card = card->_usd_smartcard_new (manager->module,
                                                slot_id, slot_series);

                card_name = card->usd_smartcard_get_name (card);

                g_hash_table_replace (manager->smartcards,
                                      card_name, card);
        }
}


bool SmartcardManager::managerStart()
{
    int worker_fd;
    GIOChannel *io_channel;
    GSource *source;
    GError *nss_error;
    if (this->state == USD_SMARTCARD_MANAGER_STATE_STARTED){
        CT_SYSLOG(LOG_DEBUG,"smartcard manager already started");
        return true;
    }
    this->state = USD_SMARTCARD_MANAGER_STATE_STARTED;
    worker_fd = -1;
    nss_error = NULL;
    if (!this->nss_is_loaded && ! load_nss(&nss_error)){
        g_propagate_error (NULL, nss_error);
        goto out;
    }
    this->nss_is_loaded = TRUE;

    if (this->module == NULL)
        this->module = load_driver(this->module_path,&nss_error);
    if (this->module == NULL){
         g_propagate_error (NULL, nss_error);
        goto out;
    }
    if (!usd_smartcard_manager_create_worker(this,&worker_fd,&this->worker_thread)){
        goto out;
    }
    io_channel = g_io_channel_unix_new (worker_fd);

    source = g_io_create_watch (io_channel, (GIOCondition)(G_IO_IN | G_IO_HUP));
    g_io_channel_unref (io_channel);
    io_channel = NULL;
    this->smartcard_event_source = source;

    g_source_set_callback (this->smartcard_event_source,
                                   (GSourceFunc) (GIOFunc)
                                   usd_smartcard_manager_check_for_and_process_events,
                                   this,
                                   (GDestroyNotify)
                                   usd_smartcard_manager_event_processing_stopped_handler);
    g_source_attach (this->smartcard_event_source, NULL);
    g_source_unref (this->smartcard_event_source);

    /* populate the hash with cards that are already inserted
     */
    usd_smartcard_manager_get_all_cards (this);

    this->state = USD_SMARTCARD_MANAGER_STATE_STARTED;

out:
    /* don't leave it in a half started state*/
    if  (this->state != USD_SMARTCARD_MANAGER_STATE_STARTED){
        CT_SYSLOG(LOG_ERR,"smartcard manager could not be completely started");
        this->managerStop();
    }
    else
        CT_SYSLOG(LOG_DEBUG,"smartcard manager started");
    return this->state == USD_SMARTCARD_MANAGER_STATE_STARTED;
}
void usd_smartcard_manager_queue_stop (SmartcardManager *manager)
{

        manager->state = USD_SMARTCARD_MANAGER_STATE_STOPPING;

        g_idle_add ((GSourceFunc) usd_smartcard_manager_stop_now, manager);
}


bool SmartcardManager::managerStop()
{
    if (this->state == USD_SMARTCARD_MANAGER_STATE_STOPPED) {
            return TRUE;
    }

    if (this->is_unstoppable) {
            usd_smartcard_manager_queue_stop (this);
            return TRUE;
    }

    usd_smartcard_manager_stop_now (this);
}

static void usd_smartcard_manager_check_for_login_card (CK_SLOT_ID   slot_id,
                                            Smartcard *card,
                                            gboolean     *is_inserted)
{
        g_assert (is_inserted != NULL);

        if (card->usd_smartcard_is_login_card (card)) {
                *is_inserted = TRUE;
        }

}

gboolean SmartcardManager::usd_smartcard_manager_login_card_is_inserted (SmartcardManager *manager)

{
        gboolean is_inserted;

        is_inserted = FALSE;
        g_hash_table_foreach (manager->smartcards,
                              (GHFunc)
                              usd_smartcard_manager_check_for_login_card,
                              &is_inserted);
        return is_inserted;
}


/*
int main(int argc, char *argv[])
{
    SmartcardManager *manager = SmartcardManager::managerNew();
    g_log_set_always_fatal ((GLogLevelFlags)(G_LOG_LEVEL_ERROR | G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_WARNING));



}
*/
