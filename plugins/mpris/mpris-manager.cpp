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
#include <QStringList>
#include <QDBusReply>
#include <QDBusMessage>
#include "mpris-manager.h"
#include <syslog.h>

const QString MPRIS_OBJECT_PATH = "/org/mpris/MediaPlayer2";
const QString MPRIS_INTERFACE = "org.mpris.MediaPlayer2.Player";
const QString MPRIS_PREFIX = "org.mpris.MediaPlayer2.";
const QString DBUS_NAME = "org.ukui.SettingsDaemon";
const QString DBUS_PATH = "/org/ukui/SettingsDaemon";
const QString MEDIAKEYS_DBUS_NAME = DBUS_NAME + ".MediaKeys";
const QString MEDIAKEYS_DBUS_PATH = DBUS_PATH + "/MediaKeys";

/* Number of media players supported.
 * Correlates to the number of elements in BUS_NAMES */
const int NUM_BUS_NAMES = 16;

/* Names to we want to watch */
const QStringList busNames ={"org.mpris.MediaPlayer2.audacious",
                             "org.mpris.MediaPlayer2.clementine",
                             "org.mpris.MediaPlayer2.vlc",
                             "org.mpris.MediaPlayer2.mpd",
                             "org.mpris.MediaPlayer2.exaile",
                             "org.mpris.MediaPlayer2.banshee",
                             "org.mpris.MediaPlayer2.rhythmbox",
                             "org.mpris.MediaPlayer2.pragha",
                             "org.mpris.MediaPlayer2.quodlibet",
                             "org.mpris.MediaPlayer2.guayadeque",
                             "org.mpris.MediaPlayer2.amarok",
                             "org.mpris.MediaPlayer2.nuvolaplayer",
                             "org.mpris.MediaPlayer2.xbmc",
                             "org.mpris.MediaPlayer2.xnoise",
                             "org.mpris.MediaPlayer2.gmusicbrowser",
                             "org.mpris.MediaPlayer2.spotify",
                            };

MprisManager* MprisManager::mMprisManager = nullptr;

MprisManager::MprisManager(QObject *parent):QObject(parent)
{

}

MprisManager::~MprisManager()
{

}

bool MprisManager::MprisManagerStart (GError           **error)
{
    QStringList list;
    QDBusConnection conn = QDBusConnection::sessionBus();
    QDBusMessage tmpMsg,response ;

    mPlayerQuque = new QQueue<QString>();
    mDbusWatcher = new QDBusServiceWatcher();

    mDbusWatcher->setWatchMode(QDBusServiceWatcher::WatchForRegistration |
                               QDBusServiceWatcher::WatchForUnregistration);
    mDbusWatcher->setConnection(conn);
    mDbusInterface = new QDBusInterface(DBUS_NAME,MEDIAKEYS_DBUS_PATH,MEDIAKEYS_DBUS_NAME,
                                        conn);

   syslog (LOG_DEBUG,"Starting mpris manager");

    /* Register all the names we wish to watch.*/
    mDbusWatcher->setWatchedServices(busNames);
    mDbusWatcher->addWatchedService(DBUS_NAME);
    connect(mDbusWatcher,SIGNAL(serviceRegistered(const QString&)),this,SLOT(serviceRegisteredSlot(const QString&)));
    connect(mDbusWatcher,SIGNAL(serviceUnregistered(const QString&)),this,SLOT(serviceUnregisteredSlot(const QString&)));

    if(!mDbusInterface->isValid()){
        syslog(LOG_ERR,"create %s failed",MEDIAKEYS_DBUS_NAME.toLatin1().data());
        return false;
    }

    /** create a dbus method call by QDBusInterface。here we call GrabMediaPlayerKeys(QString) from "org.ukui.SettingsDaemon.MediaKeys",
     *  you can find it's definition at media-keys mprismanager.cpp
     *
     *  通过QDBusInterface的方式创建一个dbus方法调用。这里我们调用的方法是来自org.ukui.SettingsDaemon.MediaKeys的
     *  GrabMediaPlayerKeys(QString), 你可以在media-keys插件内的mprismanager.cpp找到它的定义
     */
    response = mDbusInterface->call("GrabMediaPlayerKeys","UsdMpris");
    if(QDBusMessage::ErrorMessage == response.ReplyMessage){
        syslog(LOG_ERR,"error: %s",response.errorMessage().toLatin1().data());
        return false;
    }

    /** wait signal MediaPlayerKeyPressed() from "org.ukui.SettingsDaemon.MediaKeys"
     *  you can find it's definition at media-keys mprismanager.h
     *
     *  等待连接来自于org.ukui.SettingsDaemon.MediaKeys的MediaPlayerKeyPressed()信号
     *  你可以在media-keys 插件的 mrpismanager.h文件内找到它的定义
     */
    connect(mDbusInterface,SIGNAL(MediaPlayerKeyPressed(QString,QString)),
            this,SLOT(keyPressed(QString,QString)));

    return true;
}

void MprisManager::MprisManagerStop()
{
    syslog (LOG_DEBUG,"Stopping mpris manager");

    delete mDbusInterface;
    mDbusInterface = nullptr;

    delete mDbusWatcher;
    mDbusWatcher = nullptr;

    mPlayerQuque->clear();
    delete mPlayerQuque;
    mPlayerQuque = nullptr;
}

MprisManager* MprisManager::MprisManagerNew()
{
    if(nullptr == mMprisManager)
        mMprisManager = new MprisManager();
    return mMprisManager;
}

/** Returns the name of the media player.
 * @name    if @name=="org.mpris.MediaPlayer2.vlc"
 * @return  then @return=="vlc"
 */
QString getPlayerName(const QString& name)
{
    QString ret_name=name.section(".",3,4);
    return ret_name;
}

void MprisManager::serviceRegisteredSlot(const QString& service)
{
    QString realPlayername;

    syslog (LOG_DEBUG,"MPRIS Name Registered: %s\n", service.toLatin1().data());

    if(DBUS_NAME == service){
        return;
    }else{
        /* A media player was just run and should be
         * added to the head of @mPlayerQuque.
         */
        realPlayername = getPlayerName(service);
        mPlayerQuque->push_front(realPlayername);
    }
}

/**
 * @brief MprisManager::serviceUnregisteredSlot
 *        if anyone dbus service that from @busNames quit,this func will be called.
 *        如果来自于@busNames的任意一个dbus服务关闭，则该函数被执行
 * @param service
 *        for example: @service can be "org.mpris.MediaPlayer2.vlc"
 */
void MprisManager::serviceUnregisteredSlot(const QString& service)
{
    QString realPlayername;
    syslog (LOG_DEBUG,"MPRIS Name Unregistered: %s\n", service.toLatin1().data());

    if(DBUS_NAME == service){
        if(nullptr != mDbusInterface){
            delete mDbusInterface;
            mDbusInterface = nullptr;
        }
    }else{
        /* A media player quit running and should be removed from @mPlayerQueue.
         * 一个媒体播放器退出时应当从@mPlayerQueue中移除
         */
        realPlayername = getPlayerName(service);
        if(mPlayerQuque->contains(realPlayername))
            mPlayerQuque->removeOne(realPlayername);
    }
}

/**
 * @brief MprisManager::keyPressed
 *        after catch MediaPlayerKeyPressed() signal, this function will be called
 *        在接收到MediaPlayerKeyPressed() signal后，这个函数将被调用
 * @param application @application=="UsdMpris" is true
 * @param operation
 *        we control media player by @operation value，for example: play 、pause 、stop
 *        我们通过@operation的值去控制媒体播放器,如： 播放、暂停、停止
 */
void MprisManager::keyPressed(QString application,QString operation)
{
    QString mprisKey = nullptr;
    QString mprisHead,mprisName;
    QDBusMessage playerMsg,response;

    if("UsdMpris" != application)
        return;
    if(mPlayerQuque->isEmpty())
        return;

    if("Play" == operation)
        mprisKey = "PlayPause";
    else if("Pause" == operation)
        mprisKey = "Pause";
    else if("Previous" == operation)
        mprisKey = "Previous";
    else if("Next" == operation)
        mprisKey = "Next";
    else if("Stop" == operation)
        mprisKey = "Stop";

    if(mprisKey.isNull())
       return;

    mprisHead = mPlayerQuque->head();
    mprisName = MPRIS_PREFIX + mprisHead;

    /* create a dbus method call by QDBusMessage, the @mprisName is a member of @busNames
     * 通过QDBusMessage的方式创建一个dbus方法调用，@mprisName 来自于 @busNames
     */
    playerMsg = QDBusMessage::createMethodCall(mprisName,MPRIS_OBJECT_PATH,MPRIS_INTERFACE,mprisKey);
    response = QDBusConnection::sessionBus().call(playerMsg);
    if(response.type() == QDBusMessage::ErrorMessage)
        syslog(LOG_ERR,"error: %s",response.errorMessage().toLatin1().data());
}


