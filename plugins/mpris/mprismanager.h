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
#ifndef MPRISMANAGER_H
#define MPRISMANAGER_H

#include <QObject>
#include <QQueue>
#include <QString>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusConnection>

/** undef 'signals' from qt,avoid conflict with Glib
 *  undef qt中的 'signals' 关键字，避免与 Glib 冲突
 */
#ifdef signals
#undef signals
#endif

#include <gio/gio.h>        //for GError

class MprisManager : public QObject{
    Q_OBJECT
public:
    ~MprisManager();
    static MprisManager* MprisManagerNew();
    bool MprisManagerStart(GError **error);
    void MprisManagerStop();

private:
    MprisManager(QObject *parent = nullptr);
    MprisManager(const MprisManager&) = delete;

private Q_SLOTS:
    void serviceRegisteredSlot(const QString&);
    void serviceUnregisteredSlot(const QString&);
    void keyPressed(QString,QString);

private:
    static MprisManager   *mMprisManager;
    QDBusServiceWatcher   *mDbusWatcher;
    QDBusInterface        *mDbusInterface;
    QQueue<QString>       *mPlayerQuque;
};

#endif /* MPRISMANAGER_H */
