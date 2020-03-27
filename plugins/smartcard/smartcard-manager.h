#ifndef SMARTCARDMANAGER_H
#define SMARTCARDMANAGER_H

#include <QObject>
#include <QThread>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

#include <glib-object.h>

#include <nspr/prerror.h>
#include <nss/nss.h>
#include <nss/pk11func.h>
#include <nss/secmod.h>
#include <nss/secerr.h>

#include "usd-smartcard.h"

typedef struct _SmartcardManagerWorker SmartcardManagerWorker;

enum SmartcardManagerState {
        USD_SMARTCARD_MANAGER_STATE_STOPPED = 0,
        USD_SMARTCARD_MANAGER_STATE_STARTING,
        USD_SMARTCARD_MANAGER_STATE_STARTED,
        USD_SMARTCARD_MANAGER_STATE_STOPPING,
};

struct _SmartcardManagerWorker {
        SECMODModule *module;
        GHashTable *smartcards;
        int write_fd;
        guint32 nss_is_loaded : 1;
};

class Smartcard;

class SmartcardManager : public QObject
{
    Q_OBJECT

/*Q_SIGNALS:
    void smartcard_inserted();
    void smartcard_removed();*/
/*Q_SIGNALS:
    void usd_smartcard_manager_emit_smartcard_inserted (SmartcardManager *manager,
                                                        Smartcard        *card);
    void usd_smartcard_manager_emit_smartcard_removed (SmartcardManager *manager,
                                                        Smartcard        *card);*/

public Q_SLOTS:
    void usd_smartcard_manager_card_inserted_handler(Smartcard * card);
    void usd_smartcard_manager_card_removed_handler(Smartcard   *card);

public:
    ~SmartcardManager();
    bool managerStart();
    bool managerStop();
    static SmartcardManager *managerNew();
    gboolean usd_smartcard_manager_login_card_is_inserted (SmartcardManager *manager);
    void usd_smartcard_manager_class_install_properties();
    void usd_smartcard_manager_set_property();
    void usd_smartcard_manager_get_property();
    void usd_smartcard_manager_set_module_path(const char *module_path);
    char *usd_smartcard_manager_get_module_path ();
    void usd_smartcard_manager_class_install_signals ();
    void usd_smartcard_manager_emit_smartcard_inserted (SmartcardManager *manager,
                                                        Smartcard        *card);
    void usd_smartcard_manager_emit_smartcard_removed (SmartcardManager *manager,
                                                        Smartcard        *card);


private:
    SmartcardManager(QObject *parent);
    SmartcardManager()=delete;
    SmartcardManager(SmartcardManager&) = delete;
    SmartcardManager& operator = (const SmartcardManager&) = delete;

    //SmartcardManager(QObject *parent = nullptr);

    friend gboolean usd_smartcard_manager_create_worker (SmartcardManager  *manager,
                                                         int                  *worker_fd,
                                                         GThread             **worker_thread);

    friend gboolean usd_smartcard_manager_check_for_and_process_events (GIOChannel          *io_channel,
                                                                GIOCondition         condition,
                                                                SmartcardManager    *manager);

    friend gboolean usd_smartcard_manager_stop_now (SmartcardManager *manager);
    friend void usd_smartcard_manager_stop_watching_for_events (SmartcardManager  *manager);
    friend void usd_smartcard_manager_event_processing_stopped_handler (SmartcardManager *manager);
    friend void usd_smartcard_manager_get_all_cards (SmartcardManager *manager);
    friend void usd_smartcard_manager_queue_stop (SmartcardManager *manager);

private:
    static SmartcardManager *mManager;
    SmartcardManagerState state;
    SECMODModule *module;
    char        *module_path;

    GSource *smartcard_event_source;
    GPid smartcard_event_watcher_pid;
    GHashTable *smartcards;

    GThread    *worker_thread;

    guint poll_timeout_id;

    guint32 is_unstoppable : 1;
    guint32 nss_is_loaded : 1;

};

#endif // SMARTCARDMANAGER_H
