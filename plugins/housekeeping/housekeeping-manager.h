#ifndef HOUSEKEEPINGMANAGER_H
#define HOUSEKEEPINGMANAGER_H

#include <QObject>
#include <QGSettings>
#include <QApplication>


#include <gio/gio.h>
#include <glib/gstdio.h>
#include <string.h>
#include "usd-disk-space.h"

class HousekeepingManager : public QObject
{
    Q_OBJECT
// private:
public:
    HousekeepingManager();
    HousekeepingManager(HousekeepingManager&)=delete;

    ~HousekeepingManager();
    bool HousekeepingManagerStart();
    void HousekeepingManagerStop();

public Q_SLOTS:
    void settings_changed_callback(QString);

public:
    void do_cleanup_soon();
    void purge_thumbnail_cache ();
    bool do_cleanup ();
    bool do_cleanup_once ();

private:
    static HousekeepingManager *mHouseManager;
    static DIskSpace *mDisk;
    QTimer *long_term_handler;
    QTimer *short_term_handler;
    QGSettings   *settings;

};

#endif // HOUSEKEEPINGMANAGER_H
