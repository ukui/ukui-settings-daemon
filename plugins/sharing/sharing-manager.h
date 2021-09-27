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
#ifndef SHARING_MANAGER_H
#define SHARING_MANAGER_H

#include <QObject>
#include <QGSettings/qgsettings.h>
#include <QDBusConnection>
#include <QDBusServer>
#include <QDBusMessage>
#include <QDBusInterface>
#include "sharing-dbus.h"
#include "sharing-adaptor.h"

typedef enum {
       USD_SHARING_STATUS_OFFLINE,
       USD_SHARING_STATUS_DISABLED_MOBILE_BROADBAND,
       USD_SHARING_STATUS_DISABLED_LOW_SECURITY,
       USD_SHARING_STATUS_AVAILABLE
} UsdSharingStatus;

class SharingManager:public QObject
{
    Q_OBJECT
public:
    ~SharingManager();
    static SharingManager *SharingManagerNew();
    bool start();
    void stop();

public:
    bool sharingManagerStartService(QString service);
    bool sharingManagerStopService(QString service);
    bool sharingManagerHandleService(QString method, QString service);
    void sharingManagerServiceChange(QString state, QString service);
    void updateSaveService(bool state, QString serviceName);

private:
    SharingManager();
    SharingManager(const SharingManager&) = delete;

private:
    static SharingManager   *mSharingManager;
    QGSettings              *settings;
    UsdSharingStatus        sharing_status;
    sharingDbus             *mDbus;
};

#endif // SHARING_MANAGER_H
