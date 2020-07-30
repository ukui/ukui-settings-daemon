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
    void do_cleanup_soon();
    void purge_thumbnail_cache ();
    void do_cleanup ();
    void do_cleanup_once ();

private:
    static HousekeepingManager *mHouseManager;
    static DIskSpace *mDisk;
    QTimer *long_term_handler;
    QTimer *short_term_handler;
    QGSettings   *settings;

};

#endif // HOUSEKEEPINGMANAGER_H
