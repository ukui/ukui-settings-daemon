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
#ifndef MEDIAKEYSMANAGER_H
#define MEDIAKEYSMANAGER_H

#include <QObject>
#include <QTimer>
#include <QTime>
#include <QString>
#include <QProcess>
#include <QGSettings>
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QDBusConnection>
#include <QDebug>

#include "volumewindow.h"
#include "devicewindow.h"
#include "acme.h"
#include "xeventmonitor.h"
#include "pulseaudiomanager.h"

#ifdef signals
#undef signals
#endif

extern "C"{
#include <syslog.h>
#include <glib.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
//#include <libmatemixer/matemixer.h>
#include <X11/Xlib.h>
#include "ukui-input-helper.h"
}

/** this value generate from mpris serviceRegisteredSlot(const QString&)
 *  这个值一般由mpris插件的serviceRegisteredSlot(const QString&)传递
 */
typedef struct{
    QString application;
    uint time;
}MediaPlayer;

class MediaKeysManager:public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface","org.ukui.SettingsDaemon.MediaKeys")
public:
    ~MediaKeysManager();
    static MediaKeysManager* mediaKeysNew();
    bool mediaKeysStart(GError*);
    void mediaKeysStop();

private:
    MediaKeysManager(QObject* parent = nullptr);
    void initScreens();
    void initKbd();
    void initXeventMonitor();
    bool getScreenLockState();

//    static void onStreamControlVolumeNotify (MateMixerStreamControl *control,GParamSpec *pspec,MediaKeysManager *mManager);
//    static void onStreamControlMuteNotify (MateMixerStreamControl *control,GParamSpec *pspec,MediaKeysManager *mManager);
    static GdkFilterReturn acmeFilterEvents(GdkXEvent*,GdkEvent*,void*);
//    static void onContextStateNotify(MateMixerContext*,GParamSpec*,MediaKeysManager*);
//    static void onContextDefaultOutputNotify(MateMixerContext*,GParamSpec*,MediaKeysManager*);
//    static void onContextDefaultInputNotify(MateMixerContext*,GParamSpec*,MediaKeysManager*);
//    static void onContextStreamRemoved(MateMixerContext*,char*,MediaKeysManager*);
    static void updateDefaultOutput(MediaKeysManager *);
    static void updateDefaultInput(MediaKeysManager *);
    GdkScreen *acmeGetScreenFromEvent (XAnyEvent*);
    bool doAction(int);

    void initShortcuts();
    /******************Functional class function(功能类函数)****************/
    void doTouchpadAction();
    void doSoundAction(int);
    void doSoundActionALSA(int);
    void doMicSoundAction();
    void updateDialogForVolume(uint,bool,bool);
    void executeCommand(const QString&,const QString&);
    void doShutdownAction();
    void doLogoutAction();
    void doOpenHomeDirAction();
    void doSearchAction();
    void doScreensaverAction();
    void doSettingsAction();
    void doOpenFileManagerAction();
    void doMediaAction();
    void doOpenCalcAction();
    void doToggleAccessibilityKey(const QString key);
    void doMagnifierAction();
    void doScreenreaderAction();
    void doOnScreenKeyboardAction();
    void doOpenTerminalAction();
    void doOpenMonitor();
    void doOpenConnectionEditor();
    void doScreenshotAction(const QString);
    void doUrlAction(const QString);
    void doMultiMediaPlayerAction(const QString);
    void doSidebarAction();
    void doWindowSwitchAction();
    void doOpenUkuiSearchAction();
    void doOpenKdsAction();

    /******************Function for DBus(DBus相关处理函数)******************************/
    bool findMediaPlayerByApplication(const QString&);
    uint findMediaPlayerByTime(MediaPlayer*);
    void removeMediaPlayerByApplication(const QString&,uint);

public Q_SLOTS:
    /** two dbus method, will be called in mpris plugin(mprismanager.cpp MprisManagerStart())
     *  两个dbus 方法，将会在mpris插件中被调用(mprismanager.cpp MprisManagerStart())
     */
    void GrabMediaPlayerKeys(QString application);
    void ReleaseMediaPlayerKeys(QString application);

private Q_SLOTS:
    //void timeoutCallback();
    void updateKbdCallback(const QString&);
    void XkbEventsPress(const QString &keyStr);
    void XkbEventsRelease(const QString &keyStr);

Q_SIGNALS:
    /** media-keys plugin will emit this signal by org.ukui.SettingsDaemon.MediaKeys
     *  when listen some key Pressed(such as XF86AudioPlay 、XF86AudioPause 、XF86AudioForward)
     *  at the same time,mpris plugin will connect this signal
     *
     *  当监听到某些按键(XF86AudioRewind、XF86AudioRepeat、XF86AudioRandomPlay)时，
     *  media-keys插件将会通过org.ukui.SettingsDaemon.MediaKeys这个dbus发送这个signal
     *  同时，mpris插件会connect这个signal
    */
    void MediaPlayerKeyPressed(QString application,QString operation);

private:
    pulseAudioManager *mpulseAudioManager;
    static MediaKeysManager* mManager;
    QDBusMessage      mDbusScreensaveMessage;

    QTimer            *mTimer;
    QGSettings        *mSettings;
    QGSettings        *pointSettings;
    QGSettings        *sessionSettings;
    QGSettings        *shotSettings;

    QProcess          *mExecCmd;
    GdkScreen         *mCurrentScreen;  //current GdkScreen

//    MateMixerStream   *mStream;
//    MateMixerContext  *mContext;
//    MateMixerStreamControl  *mControl;
//    MateMixerStream   *mInputStream;
//    MateMixerStreamControl  *mInputControl;

    VolumeWindow      *mVolumeWindow;   //volume size window 声音大小窗口
    DeviceWindow      *mDeviceWindow;   //other widow，such as touchapad、volume 例如触摸板、磁盘卷设备
    QList<MediaPlayer*> mediaPlayers;   //all opened media player(vlc,audacious) 已经打开的媒体播放器列表(vlc,audacious)

    bool               m_ctrlFlag = false;
};

#endif // MEDIAKEYSMANAGER_H
