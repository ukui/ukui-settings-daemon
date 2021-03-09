/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
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

#include <ldsm-trash-empty.h>
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
    bool ldsm_mount_is_user_ignore (const char *path);
    bool ldsm_mount_should_ignore (GUnixMountEntry *mount);
    bool ldsm_mount_has_space (LdsmMountInfo *mount);
    void ldsm_maybe_warn_mounts (GList *mounts,
                                 bool multiple_volumes,
                                 bool other_usable_volumes);
    bool ldsm_notify_for_mount (LdsmMountInfo *mount,
                                bool       multiple_volumes,
                                bool       other_usable_volumes);
    static void ldsm_mounts_changed (GObject  *monitor,gpointer  data,DIskSpace *disk);

public Q_SLOTS:
    void usdLdsmUpdateConfig(QString);
    bool ldsm_check_all_mounts();

private:
    void cleanNotifyHash();
    DIskSpace         *mDisk;
    GHashTable        *ldsm_notified_hash ;
    QHash<const char*, LdsmMountInfo*> m_notified_hash;
    QTimer            *ldsm_timeout_cb;
    GUnixMountMonitor *ldsm_monitor;
    double             free_percent_notify;
    double             free_percent_notify_again;
    unsigned int       free_size_gb_no_notify;
    unsigned int       min_notify_period;
    GSList            *ignore_paths;
    QGSettings        *settings;
    LdsmDialog        *dialog;
    LdsmTrashEmpty    *trash_empty;
    QVariantList       ignoreList;
    bool               done;

};

#endif // DISKSPACE_H
