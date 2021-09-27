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

#include "sharing-manager.h"
#include "clib-syslog.h"


#define SHATINGS_SCHEMA     "org.ukui.SettingsDaemon.plugins.sharing"
#define KEY_SERVICE_NAME    "service-name"

#define USD_DBUS_NAME           "org.ukui.SettingsDaemon"
#define USD_DBUS_PATH           "/org/ukui/SettingsDaemon"
#define USD_DBUS_BASE_INTERFACE "org.ukui.SettingsDaemon"

#define RYGEL_BUS_NAME        "org.ukui.Rygel1"
#define RYGEL_OBJECT_PATH     "/org/ukui/Rygel1"
#define RYGEL_INTERFACE_NAME  "org.ukui.Rygel1"

#define SYSTEM_DBUS_NAME    "org.freedesktop.systemd1"
#define SYSTEM_DBUS_PATH    "/org/freedesktop/systemd1"
#define SYSTEM_DBUS_INTER   "org.freedesktop.systemd1.Manager"

#define USD_SHARING_DBUS_NAME USD_DBUS_NAME ".Sharing"
#define USD_SHARING_DBUS_PATH USD_DBUS_PATH "/Sharing"

SharingManager* SharingManager::mSharingManager = nullptr;

SharingManager::SharingManager()
{
    mDbus = new sharingDbus(this);
    new SharingAdaptor(mDbus);
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if(sessionBus.registerService(USD_DBUS_NAME)){
        sessionBus.registerObject(USD_SHARING_DBUS_PATH,
                                  mDbus,
                                  QDBusConnection::ExportAllContents);
    }
    connect(mDbus, &sharingDbus::serviceChange,
            this, &SharingManager::sharingManagerServiceChange);
}

SharingManager::~SharingManager()
{
    if(mSharingManager)
        delete mSharingManager;
}

SharingManager  *SharingManager::SharingManagerNew()
{
    if(nullptr == mSharingManager)
        mSharingManager = new SharingManager();
    return mSharingManager;
}

bool SharingManager::sharingManagerStartService(QString service)
{
    bool serviceState;
    USD_LOG(LOG_DEBUG, "About to start %s", service.toLatin1().data());
    serviceState = sharingManagerHandleService("StartUnit", service);
    return serviceState;
}

bool SharingManager::sharingManagerStopService(QString service)
{
    bool serviceState;
    USD_LOG(LOG_DEBUG, "About to stop %s", service.toLatin1().data());
    serviceState = sharingManagerHandleService("StopUnit", service);
    return serviceState;
}

bool SharingManager::sharingManagerHandleService(QString method, QString service )
{
    QString service_file = QString("%1.service").arg(service);
    QDBusMessage message =
    QDBusMessage::createMethodCall(SYSTEM_DBUS_NAME,
                                   SYSTEM_DBUS_PATH,
                                   SYSTEM_DBUS_INTER,
                                   method);
    message <<service_file<<"replace";
    QDBusMessage response = QDBusConnection::sessionBus().call(message);
    if (response.type() != QDBusMessage::ReplyMessage){
        USD_LOG(LOG_DEBUG, "servives dbus called failed");
        return false;
    }
    return true;
}

//update gsettings "service-name" key contents
bool update_ignore_paths(QList<QString>** ignore_paths,QString serviceName,bool state)
{
    bool found=(*ignore_paths)->contains(serviceName.toLatin1().data());
    if(state && found==false){
        (*ignore_paths)->push_front(serviceName.toLatin1().data());
        return true;
    }
    if(!state && found==true){
        (*ignore_paths)->removeOne(serviceName.toLatin1().data());
        return true;
    }
    return false;
}

void SharingManager::updateSaveService(bool state, QString serviceName)
{
    QStringList service_list;
    QStringList serviceStr;
    QList<QString>* service;
    bool updated;
    QList<QString>::iterator l;

    service =new QList<QString>();
    //get contents from "ignore-paths" key
    if (!settings->get(KEY_SERVICE_NAME).toStringList().isEmpty())
        service_list.append(settings->get(KEY_SERVICE_NAME).toStringList());

    for(auto str: service_list){
        if(!str.isEmpty())
            service->push_back(str);
    }

    updated = update_ignore_paths(&service, serviceName, state);

    if(updated){
        for(l = service->begin(); l != service->end(); ++l){
            serviceStr.append(*l);
        }
        //set latest contents to gsettings "ignore-paths" key
        settings->set(KEY_SERVICE_NAME, QVariant::fromValue(serviceStr));
    }
    //free QList Memory
    if(service){
        service->clear();
    }
}

void SharingManager::sharingManagerServiceChange(QString state, QString service)
{
    bool serviceState;
    if(state.compare("enable") == 0){
        serviceState = sharingManagerStartService(service);
        if (serviceState)
            updateSaveService(true, service);
    }
    else if (state.compare("disable") == 0) {
        serviceState = sharingManagerStopService(service);
        if (serviceState)
            updateSaveService(false, service);
    }
}

bool SharingManager::start()
{
    USD_LOG(LOG_DEBUG,"Starting sharing manager!");

    settings = new QGSettings(SHATINGS_SCHEMA);
    QStringList serviceList = settings->get(KEY_SERVICE_NAME).toStringList();
    for (QString serviceName : serviceList) {
        sharingManagerStartService(serviceName);
    }
    return true;
}

void SharingManager::stop()
{
    USD_LOG(LOG_DEBUG,"Stopping sharing manager!");
    if(settings)
        delete settings;
}
