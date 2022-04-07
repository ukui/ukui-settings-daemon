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
#include <QVariant>
#include <QFileInfo>
#include <QDir>
#include <QList>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>

#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/log.h>
#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/edid.h>
#include <KF5/KScreen/kscreen/configmonitor.h>
#include <KF5/KScreen/kscreen/getconfigoperation.h>

#include "volumewindow.h"
#include "devicewindow.h"
#include "acme.h"
#include "pulseaudiomanager.h"
#include "usd_base_class.h"

#include "usd_global_define.h"
#include "xEventMonitor.h"


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
    void sjhKeyTest();
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
    void getConfigMonitor();
    /******************Functional class function(功能类函数)****************/
    void doTouchpadAction(int);
    void doSoundAction(int);
    void doSoundActionALSA(int);
    void doMicSoundAction();
    void doBrightAction(int);
    void updateDialogForVolume(uint,bool,bool);
    void executeCommand(const QString&,const QString&);
    void doShutdownAction();
    void doLogoutAction();
    void doPowerOffAction();
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
    void doWlanAction();
    void doWebcamAction();
    void doEyeCenterAction();
    void doFlightModeAction();
    void doOpenKylinCalculator();
    void doBluetoothAction();
    void doOpenEvolutionAction();


    /******************Function for DBus(DBus相关处理函数)******************************/
    bool findMediaPlayerByApplication(const QString&);
    uint findMediaPlayerByTime(MediaPlayer*);
    void removeMediaPlayerByApplication(const QString&,uint);
    int8_t getCurrentMode();

public Q_SLOTS:
    /** two dbus method, will be called in mpris plugin(mprismanager.cpp MprisManagerStart())
     *  两个dbus 方法，将会在mpris插件中被调用(mprismanager.cpp MprisManagerStart())
     */
    void GrabMediaPlayerKeys(QString application);
    void ReleaseMediaPlayerKeys(QString application);

    int  getFlightState();
    void setFlightState(int value);
    void mediaKeyForOtherApp(int action,QString appName);
private Q_SLOTS:
    //void timeoutCallback();
    void updateKbdCallback(const QString&);
    void XkbEventsPress(const QString &keyStr);
    void XkbEventsRelease(const QString &keyStr);
    void MMhandleRecordEvent(xEvent* data);
    void MMhandleRecordEventRelease(xEvent* data);

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
    QDBusInterface*   m_dbusControlCenter;

    KScreen::ConfigPtr m_config;

    QTimer            *mTimer;
    QGSettings        *mSettings;
    QGSettings        *pointSettings;
    QGSettings        *sessionSettings;
    QGSettings        *shotSettings;
    QGSettings        *powerSettings;

    QProcess          *mExecCmd;
    GdkScreen         *mCurrentScreen;  //current GdkScreen
    xEventMonitor *mXEventMonitor = nullptr;

//    MateMixerStream   *mStream;
//    MateMixerContext  *mContext;
//    MateMixerStreamControl  *mControl;
//    MateMixerStream   *mInputStream;
//    MateMixerStreamControl  *mInputControl;

    VolumeWindow      *mVolumeWindow;   //volume size window 声音大小窗口
    DeviceWindow      *mDeviceWindow;   //other widow，such as touchapad、volume 例如触摸板、磁盘卷设备
    QList<MediaPlayer*> mediaPlayers;   //all opened media player(vlc,audacious) 已经打开的媒体播放器列表(vlc,audacious)
    int                power_state = 4;
    bool               m_ctrlFlag = false;

    bool               m_isNoteBook;
    int                m_prevPrimaryOutputId;
    QString            m_edidHash;

    xEventHandleHadRelase(MUTE_KEY);
    xEventHandleHadRelase(AREA_SCREENSHOT_KEY);
    xEventHandleHadRelase(WINDOW_SCREENSHOT_KEY);
    xEventHandleHadRelase(SCREENSHOT_KEY);
    xEventHandleHadRelase(WLAN_KEY);
    xEventHandleHadRelase(MIC_MUTE_KEY);
    xEventHandleHadRelase(RFKILL_KEY);
    xEventHandleHadRelase(TOUCHPAD_KEY);
    xEventHandleHadRelase(TOUCHPAD_ON_KEY);
    xEventHandleHadRelase(TOUCHPAD_OFF_KEY);
    xEventHandleHadRelase(SCREENSAVER_KEY);
    xEventHandleHadRelase(WINDOWSWITCH_KEY);
    xEventHandleHadRelase(CALCULATOR_KEY);
    xEventHandleHadRelase(BLUETOOTH_KEY);



};

#endif // MEDIAKEYSMANAGER_H
