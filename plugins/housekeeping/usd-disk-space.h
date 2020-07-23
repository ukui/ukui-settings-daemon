#ifndef DISKSPACE_H
#define DISKSPACE_H

#include <QObject>
//#include <QApplication>
#include <QGSettings>

#include "config.h"

#include <sys/statvfs.h>
#include <time.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib-object.h>
#include <gio/gunixmounts.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "usd-ldsm-dialog.h"

#include <qhash.h>
class QGSettings;
typedef struct
{
    GUnixMountEntry *mount;
    struct statvfs buf;
    time_t notify_time;
} LdsmMountInfo;

class DIskSpace :  public QObject
{
    Q_OBJECT
public:
    DIskSpace();

    ~DIskSpace();
    void UsdLdsmSetup (bool check_now);
    void UsdLdsmClean ();
    void usdLdsmGetConfig ();
    static bool ldsm_mount_is_user_ignore (const char *path);
    static void ldsm_mounts_changed (GObject  *monitor,gpointer  data);
    static bool        ldsm_check_all_mounts ();
    static bool ldsm_mount_should_ignore (GUnixMountEntry *mount);
    static bool ldsm_mount_has_space (LdsmMountInfo *mount);
    static void ldsm_maybe_warn_mounts (GList *mounts,
                                        bool multiple_volumes,
                                        bool other_usable_volumes);
    static bool ldsm_notify_for_mount (LdsmMountInfo *mount,
                                       bool       multiple_volumes,
                                       bool       other_usable_volumes);

public Q_SLOTS:
    void usdLdsmUpdateConfig(QString);

private:
    void cleanNotifyHash();
    static DIskSpace *mDisk;
    static GHashTable        *ldsm_notified_hash ;
    static QHash<const char*, LdsmMountInfo*> m_notified_hash;
    static QTimer*       ldsm_timeout_cb;

    static GUnixMountMonitor *ldsm_monitor;
    static double             free_percent_notify;
    static double             free_percent_notify_again;
    static unsigned int       free_size_gb_no_notify;
    static unsigned int       min_notify_period;
    static GSList            *ignore_paths;
    static QGSettings        *settings;
    static LdsmDialog        *dialog;
    QVariantList             ignoreList;
};

#endif // DISKSPACE_H
