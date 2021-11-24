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

#include "mediakey-manager.h"
#include "eggaccelerators.h"

#include <KF5/KGlobalAccel/KGlobalAccel>

#include "clib-syslog.h"
#include "rfkillswitch.h"

#include "usd_base_class.h"

MediaKeysManager* MediaKeysManager::mManager = nullptr;

const int VOLUMESTEP = 6;
#define midValue(x,low,high) (((x) > (high)) ? (high): (((x) < (low)) ? (low) : (x)))
#define TIME_LIMIT          2500
#define MAX_BRIGHTNESS      100
#define STEP_BRIGHTNESS     10

#define UKUI_DAEMON_NAME        "ukui-settings-daemon"
#define MEDIAKEY_SCHEMA         "org.ukui.SettingsDaemon.plugins.media-keys"

#define POINTER_SCHEMA          "org.ukui.SettingsDaemon.plugins.mouse"
#define POINTER_KEY             "locate-pointer"

#define POWER_SCHEMA            "org.ukui.power-manager"
#define POWER_BUTTON_KEY        "button-power"

#define SESSION_SCHEMA          "org.ukui.session"
#define SESSION_WIN_KEY         "win-key-release"

#define SHOT_SCHEMA             "org.ukui.screenshot"
#define SHOT_RUN_KEY            "isrunning"

#define PANEL_QUICK_OPERATION   "org.ukui.quick-operation.panel"
#define PANEL_SOUND_STATE       "soundstate"
#define PANEL_SOUND_VOLUMSIZE   "volumesize"

#define GPM_SETTINGS_SCHEMA		            "org.ukui.power-manager"
#define GPM_SETTINGS_BRIGHTNESS_AC			"brightness-ac"

typedef enum {
    POWER_SUSPEND = 1,
    POWER_SHUTDOWN = 2,
    POWER_HIBERNATE = 3,
    POWER_INTER_ACTIVE = 4
} PowerButton;

MediaKeysManager::MediaKeysManager(QObject* parent):QObject(parent)
{
    mTimer = new QTimer(this);


    mVolumeWindow = new VolumeWindow();
    mDeviceWindow = new DeviceWindow();
    powerSettings = new QGSettings(POWER_SCHEMA);

    mSettings = new QGSettings(MEDIAKEY_SCHEMA);
    pointSettings = new QGSettings(POINTER_SCHEMA);
    sessionSettings = new QGSettings(SESSION_SCHEMA);


    gdk_init(NULL,NULL);
    //session bus 会话总线
    QDBusConnection sessionBus = QDBusConnection::sessionBus();
    if(sessionBus.registerService("org.ukui.SettingsDaemon")){
        sessionBus.registerObject("/org/ukui/SettingsDaemon/MediaKeys",this,
                                  QDBusConnection::ExportAllContents);
    }

    mXEventMonitor = new xEventMonitor(this);
}

MediaKeysManager::~MediaKeysManager()
{
    delete mTimer;

    if (mXEventMonitor) {
        mXEventMonitor->deleteLater();
    }

    if (mSettings) {
        delete mSettings;
        mSettings = nullptr;
    }
    if (pointSettings) {
        delete pointSettings;
        mSettings = nullptr;
    }
    if (sessionSettings) {
        delete sessionSettings;
        sessionSettings = nullptr;
    }
    if (shotSettings) {
        delete shotSettings;
        shotSettings = nullptr;
    }
    //  if (mExecCmd)
    //     delete mExecCmd;
    if (mVolumeWindow) {
        delete mVolumeWindow;
        mVolumeWindow = nullptr;
    }
    if (mDeviceWindow) {
        delete mDeviceWindow;
        mDeviceWindow = nullptr;
    }
    if (powerSettings) {
        delete powerSettings;
        powerSettings = nullptr;
    }

}


MediaKeysManager* MediaKeysManager::mediaKeysNew()
{
    if(nullptr == mManager)
        mManager = new MediaKeysManager();
    return mManager;
}

void MediaKeysManager::sjhKeyTest()
{
    QList<QVariant> args;
    QString param = QString::fromLocal8Bit(""
                                           "["
                                           "{"
                                           "\"enabled\": true,"
                                           " \"id\": \"e3fa3cd9190f27820ab7c30a34b9f1fb\","
                                           " \"metadata\": {"
                                           "\"fullname\": \"xrandr-DO NOT USE - RTK-WCS Display\","
                                           "\"name\": \"HDMI-1\""
                                           " },"
                                           " \"mode\": {"
                                           " \"refresh\": 30,"
                                           "\"size\": {"
                                           "  \"height\": 2160,"
                                           "  \"width\": 3840"
                                           "}"
                                           "},"
                                           "\"pos\": {"
                                           "   \"x\": 0,"
                                           "  \"y\": 0"
                                           "},"
                                           "\"primary\": false,"
                                           "\"rotation\": 1,"
                                           "\"scale\": 1"
                                           "},"
                                           "{"
                                           "   \"enabled\": true,"
                                           "  \"id\": \"e2add05191c5c70db7824c9cd76e19f5\","
                                           " \"metadata\": {"
                                           "    \"fullname\": \"xrandr-Lenovo Group Limited-LEN LI2224A-U5619HB8\","
                                           "   \"name\": \"DP-2\""
                                           "},"
                                           "\"mode\": {"
                                           "   \"refresh\": 59.93387985229492,"
                                           "  \"size\": {"
                                           "     \"height\": 1080,"
                                           "    \"width\": 1920"
                                           "}"
                                           "},"
                                           "\"pos\": {"
                                           "   \"x\": 3840,"
                                           "  \"y\": 0"
                                           "},"
                                           "\"primary\": true,"
                                           "\"rotation\": 1,"
                                           "\"scale\": 1"
                                           "}"
                                           "]"
                                           "");

    QDBusMessage message = QDBusMessage::createMethodCall(DBUS_XRANDR_NAME,
                                                          DBUS_XRANDR_PATH,
                                                          DBUS_XRANDR_INTERFACE,
                                                          "setScreensParam");

    args.append(param);
    args.append(qAppName());
    message.setArguments(args);

    QDBusConnection::sessionBus().send(message);
}

bool MediaKeysManager::getScreenLockState()
{
    bool res = false;
    QDBusMessage response = QDBusConnection::sessionBus().call(mDbusScreensaveMessage);
    if (response.type() == QDBusMessage::ReplyMessage)
    {
        if(response.arguments().isEmpty() == false) {
            bool value = response.arguments().takeFirst().toBool();
            res = value;
        }
    } else {
        USD_LOG(LOG_DEBUG, "GetLockState called failed");
    }
    return res;
}


bool MediaKeysManager::mediaKeysStart(GError*)
{
//    mate_mixer_init();
    QList<GdkScreen*>::iterator l,begin,end;

    if (true == QGSettings::isSchemaInstalled(SHOT_SCHEMA)) {
        shotSettings = new QGSettings(SHOT_SCHEMA);
        if (nullptr != shotSettings) {
            if (shotSettings->keys().contains(SHOT_RUN_KEY)) {
                if (shotSettings->get(SHOT_RUN_KEY).toBool())
                    shotSettings->set(SHOT_RUN_KEY, false);
            }
        }
    }

    mVolumeWindow->initWindowInfo();
    mDeviceWindow->initWindowInfo();

    initShortcuts();
    initXeventMonitor();

    mDbusScreensaveMessage = QDBusMessage::createMethodCall("org.ukui.ScreenSaver",
                                                            "/",
                                                            "org.ukui.ScreenSaver",
                                                            "GetLockState");

    return true;
}

int8_t MediaKeysManager::getCurrentMode()
{
    QDBusMessage message = QDBusMessage::createMethodCall(DBUS_STATUSMANAGER_NAME,
                                                          DBUS_STATUSMANAGER_PATH,
                                                          DBUS_STATUSMANAGER_NAME,
                                                          DBUS_STATUSMANAGER_GET_MODE);

    QDBusMessage response = QDBusConnection::sessionBus().call(message);

    if (response.type() == QDBusMessage::ReplyMessage) {
        if(response.arguments().isEmpty() == false) {
            bool value = response.arguments().takeFirst().toBool();
            USD_LOG(LOG_DEBUG, "get mode :%d", value);
            return value;
        }
    }

    return -1;
}

void MediaKeysManager::initShortcuts()
{

    /* WebCam */
    QAction *webCam= new QAction(this);
    webCam->setObjectName(QStringLiteral("Toggle WebCam"));
    webCam->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(webCam, QList<QKeySequence>{Qt::Key_WebCam});
    KGlobalAccel::self()->setShortcut(webCam, QList<QKeySequence>{Qt::Key_WebCam});
    connect(webCam, &QAction::triggered, this, [this]() {
        doAction(WEBCAM_KEY);
    });

    if (false == UsdBaseClass::isUseXEventAsShutKey()) {
        /* touchpad */
        QAction *touchpad= new QAction(this);
        touchpad->setObjectName(QStringLiteral("Toggle touchpad"));
        touchpad->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(touchpad, QList<QKeySequence>{Qt::Key_TouchpadToggle});
        KGlobalAccel::self()->setShortcut(touchpad, QList<QKeySequence>{Qt::Key_TouchpadToggle});
        connect(touchpad, &QAction::triggered, this, [this]() {
            doAction(TOUCHPAD_KEY);
        });

        /* Brightness Down */
        QAction *brightDown= new QAction(this);
        brightDown->setObjectName(QStringLiteral("Brightness down"));
        brightDown->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(brightDown, QList<QKeySequence>{Qt::Key_MonBrightnessDown});
        KGlobalAccel::self()->setShortcut(brightDown, QList<QKeySequence>{Qt::Key_MonBrightnessDown});
        connect(brightDown, &QAction::triggered, this, [this]() {
            USD_LOG(LOG_DEBUG,"Brightness down...............");
            doAction(BRIGHT_DOWN_KEY);
        });

        /* Brightness Up */
        QAction *brightUp = new QAction(this);
        brightUp->setObjectName(QStringLiteral("Brightness Up"));
        brightUp->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(brightUp, QList<QKeySequence>{Qt::Key_MonBrightnessUp});
        KGlobalAccel::self()->setShortcut(brightUp, QList<QKeySequence>{Qt::Key_MonBrightnessUp});
        connect(brightUp, &QAction::triggered, this, [this]() {
            USD_LOG(LOG_DEBUG,"Brightness Up ..................");
            doAction(BRIGHT_UP_KEY);
        });

        /* sound mute*/
        QAction *volumeMute= new QAction(this);
        volumeMute->setObjectName(QStringLiteral("Volume mute"));
        volumeMute->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(volumeMute, QList<QKeySequence>{Qt::Key_VolumeMute});
        KGlobalAccel::self()->setShortcut(volumeMute, QList<QKeySequence>{Qt::Key_VolumeMute});
        connect(volumeMute, &QAction::triggered, this, [this]() {
            doAction(MUTE_KEY);
        });

        /*sound down*/
        QAction *volumeDown= new QAction(this);
        volumeDown->setObjectName(QStringLiteral("Volume down"));
        volumeDown->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(volumeDown, QList<QKeySequence>{Qt::Key_VolumeDown});
        KGlobalAccel::self()->setShortcut(volumeDown, QList<QKeySequence>{Qt::Key_VolumeDown});
        connect(volumeDown, &QAction::triggered, this, [this]() {
            doAction(VOLUME_DOWN_KEY);
        });

        /*sound up*/
        QAction *volumeUp= new QAction(this);
        volumeUp->setObjectName(QStringLiteral("Volume up"));
        volumeUp->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(volumeUp, QList<QKeySequence>{Qt::Key_VolumeUp});
        KGlobalAccel::self()->setShortcut(volumeUp, QList<QKeySequence>{Qt::Key_VolumeUp});
        connect(volumeUp, &QAction::triggered, this, [this]() {
            doAction(VOLUME_UP_KEY);
        });

        /*screenshot*/
        QAction *screenshot= new QAction(this);
        screenshot->setObjectName(QStringLiteral("Take a screenshot"));
        screenshot->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(screenshot, QList<QKeySequence>{Qt::Key_Print});
        KGlobalAccel::self()->setShortcut(screenshot, QList<QKeySequence>{Qt::Key_Print});
        connect(screenshot, &QAction::triggered, this, [this]() {
            if(!mTimer->isActive()){
                mTimer->singleShot(1000, this, [=]() {
                    doAction(SCREENSHOT_KEY);
                });
            }
        });

        /*mic mute*/
        QAction *micMute= new QAction(this);
        micMute->setObjectName(QStringLiteral("Mic mute"));
        micMute->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(micMute, QList<QKeySequence>{Qt::Key_MicMute});
        KGlobalAccel::self()->setShortcut(micMute, QList<QKeySequence>{Qt::Key_MicMute});
        connect(micMute, &QAction::triggered, this, [this]() {
            doAction(MIC_MUTE_KEY);
        });


        /*window screenshot*/
        QAction *wScreenshot= new QAction(this);
        wScreenshot->setObjectName(QStringLiteral("Take a screenshot of a window"));
        wScreenshot->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(wScreenshot, QList<QKeySequence>{Qt::CTRL + Qt::Key_Print});
        KGlobalAccel::self()->setShortcut(wScreenshot, QList<QKeySequence>{Qt::CTRL + Qt::Key_Print});
        connect(wScreenshot, &QAction::triggered, this, [this]() {
            if(!mTimer->isActive()){
                mTimer->singleShot(1000, this, [=]() {
                    doAction(WINDOW_SCREENSHOT_KEY);
                });
            }
        });
        /*area screenshot*/
        QAction *aScreenshot= new QAction(this);
        aScreenshot->setObjectName(QStringLiteral("Take a screenshot of an area"));
        aScreenshot->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(aScreenshot, QList<QKeySequence>{Qt::SHIFT + Qt::Key_Print});
        KGlobalAccel::self()->setShortcut(aScreenshot, QList<QKeySequence>{Qt::SHIFT + Qt::Key_Print});
        connect(aScreenshot, &QAction::triggered, this, [this]() {
            if(!mTimer->isActive()){
                mTimer->singleShot(1000, this, [=]() {
                    doAction(AREA_SCREENSHOT_KEY);
                });
            }
        });
        /* WLAN */
        QAction *wlan= new QAction(this);
        wlan->setObjectName(QStringLiteral("Toggle wlan"));
        wlan->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(wlan, QList<QKeySequence>{Qt::Key_WLAN});
        KGlobalAccel::self()->setShortcut(wlan, QList<QKeySequence>{Qt::Key_WLAN});
        connect(wlan, &QAction::triggered, this, [this]() {
            doAction(WLAN_KEY);
        });

    }

    /*shutdown*/
    QAction *powerDown= new QAction(this);
    powerDown->setObjectName(QStringLiteral("Shut down"));
    powerDown->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(powerDown, QList<QKeySequence>{Qt::Key_PowerDown});
    KGlobalAccel::self()->setShortcut(powerDown, QList<QKeySequence>{Qt::Key_PowerDown});
    connect(powerDown, &QAction::triggered, this, [this]() {
        doAction(POWER_KEY);
    });
    /*TODO eject*/
    QAction *eject= new QAction(this);
    eject->setObjectName(QStringLiteral("Eject"));
    eject->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(eject, QList<QKeySequence>{Qt::Key_Eject});
    KGlobalAccel::self()->setShortcut(eject, QList<QKeySequence>{Qt::Key_Eject});
    connect(eject, &QAction::triggered, this, [this]() {
        doAction(EJECT_KEY);
    });
    /*home*/
    QAction *home= new QAction(this);
    home->setObjectName(QStringLiteral("Home folder"));
    home->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(home, QList<QKeySequence>{Qt::Key_Explorer});
    KGlobalAccel::self()->setShortcut(home, QList<QKeySequence>{Qt::Key_Explorer});
    connect(home, &QAction::triggered, this, [this]() {
        doAction(HOME_KEY);
    });
    /*media*/
    QAction *media= new QAction(this);
    media->setObjectName(QStringLiteral("Launch media player"));
    media->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(media, QList<QKeySequence>{Qt::Key_LaunchMedia});
    KGlobalAccel::self()->setShortcut(media, QList<QKeySequence>{Qt::Key_LaunchMedia});
    connect(media, &QAction::triggered, this, [this]() {
        doAction(MEDIA_KEY);
    });
    /*calculator*/
    QAction *cal= new QAction(this);
    cal->setObjectName(QStringLiteral("Open calculator"));
    cal->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(cal, QList<QKeySequence>{Qt::Key_Calculator});
    KGlobalAccel::self()->setShortcut(cal, QList<QKeySequence>{Qt::Key_Calculator});
    connect(cal, &QAction::triggered, this, [this]() {
        doAction(CALCULATOR_KEY);
    });
    /*search*/
    QAction *search= new QAction(this);
    search->setObjectName(QStringLiteral("Open search"));
    search->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(search, QList<QKeySequence>{Qt::Key_Search});
    KGlobalAccel::self()->setShortcut(search, QList<QKeySequence>{Qt::Key_Search});
    connect(search, &QAction::triggered, this, [this]() {
        doAction(GLOBAL_SEARCH_KEY);
    });
    /*email*/
    QAction *mail= new QAction(this);
    mail->setObjectName(QStringLiteral("Launch email client"));
    mail->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(mail, QList<QKeySequence>{Qt::Key_MailForward});
    KGlobalAccel::self()->setShortcut(mail, QList<QKeySequence>{Qt::Key_MailForward});
    connect(mail, &QAction::triggered, this, [this]() {
        doAction(EMAIL_KEY);
    });

    if (false == UsdBaseClass::isTablet()) {
        /*screensaver*/
        QAction *screensaver= new QAction(this);
        screensaver->setObjectName(QStringLiteral("Lock screen"));
        screensaver->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(screensaver, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_L});
        KGlobalAccel::self()->setShortcut(screensaver, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_L});
        connect(screensaver, &QAction::triggered, this, [this]() {
            doAction(SCREENSAVER_KEY);
        });

        /*peony2*/
        QAction *peony2= new QAction(this);
        peony2->setObjectName(QStringLiteral("Open File manager "));
        peony2->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(peony2, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_E});
        KGlobalAccel::self()->setShortcut(peony2, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_E});
        connect(peony2, &QAction::triggered, this, [this]() {
            doAction(FILE_MANAGER_KEY_2);
        });

        /*terminal*/
        QAction *terminal= new QAction(this);
        terminal->setObjectName(QStringLiteral("Open terminal"));
        terminal->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
        KGlobalAccel::self()->setDefaultShortcut(terminal, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_T});
        KGlobalAccel::self()->setShortcut(terminal, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_T});
        connect(terminal, &QAction::triggered, this, [this]() {
            doAction(TERMINAL_KEY);
        });
    }

    /*screensaver2*/
    QAction *screensaver2= new QAction(this);
    screensaver2->setObjectName(QStringLiteral("Lock screens"));
    screensaver2->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(screensaver2, QList<QKeySequence>{Qt::META + Qt::Key_L});
    KGlobalAccel::self()->setShortcut(screensaver2, QList<QKeySequence>{Qt::META + Qt::Key_L});
    connect(screensaver2, &QAction::triggered, this, [this]() {
        doAction(SCREENSAVER_KEY_2);
    });
    /*ukui-control-center*/
    QAction *ukuiControlCenter= new QAction(this);
    ukuiControlCenter->setObjectName(QStringLiteral("Open system setting"));
    ukuiControlCenter->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(ukuiControlCenter, QList<QKeySequence>{Qt::META + Qt::Key_I});
    KGlobalAccel::self()->setShortcut(ukuiControlCenter, QList<QKeySequence>{Qt::META + Qt::Key_I});
    connect(ukuiControlCenter, &QAction::triggered, this, [this]() {
        doAction(SETTINGS_KEY);
    });
    /*ukui-control-center2*/
    QAction *ukuiControlCenter2= new QAction(this);
    ukuiControlCenter2->setObjectName(QStringLiteral("Open system settings"));
    ukuiControlCenter2->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(ukuiControlCenter2, QList<QKeySequence>{Qt::Key_Tools});
    KGlobalAccel::self()->setShortcut(ukuiControlCenter2, QList<QKeySequence>{Qt::Key_Tools});
    connect(ukuiControlCenter2, &QAction::triggered, this, [this]() {
        doAction(SETTINGS_KEY_2);
    });

    /*peony*/
    QAction *peony= new QAction(this);
    peony->setObjectName(QStringLiteral("Open file manager"));
    peony->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(peony, QList<QKeySequence>{Qt::META + Qt::Key_E});
    KGlobalAccel::self()->setShortcut(peony, QList<QKeySequence>{Qt::META + Qt::Key_E});
    connect(peony, &QAction::triggered, this, [this]() {
        doAction(FILE_MANAGER_KEY);
    });

    /*help*/
    QAction *help= new QAction(this);
    help->setObjectName(QStringLiteral("Launch help browser"));
    help->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(help, QList<QKeySequence>{Qt::Key_Help});
    KGlobalAccel::self()->setShortcut(help, QList<QKeySequence>{Qt::Key_Help});
    connect(help, &QAction::triggered, this, [this]() {
        doAction(HELP_KEY);
    });
    /*www*/
    QAction *www= new QAction(this);
    www->setObjectName(QStringLiteral("Launch help browser2"));
    www->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(www, QList<QKeySequence>{Qt::Key_WWW});
    KGlobalAccel::self()->setShortcut(www, QList<QKeySequence>{Qt::Key_WWW});
    connect(www, &QAction::triggered, this, [this]() {
        doAction(WWW_KEY);
    });

    /*media play*/
    QAction *mediaPlay= new QAction(this);
    mediaPlay->setObjectName(QStringLiteral("Play/Pause"));
    mediaPlay->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(mediaPlay, QList<QKeySequence>{Qt::Key_MediaPlay});
    KGlobalAccel::self()->setShortcut(mediaPlay, QList<QKeySequence>{Qt::Key_MediaPlay});
    connect(mediaPlay, &QAction::triggered, this, [this]() {
        doAction(PLAY_KEY);
    });
    /*media pause*/
    QAction *mediaPause= new QAction(this);
    mediaPause->setObjectName(QStringLiteral("Pause playback"));
    mediaPause->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(mediaPause, QList<QKeySequence>{Qt::Key_MediaPause});
    KGlobalAccel::self()->setShortcut(mediaPause, QList<QKeySequence>{Qt::Key_MediaPause});
    connect(mediaPause, &QAction::triggered, this, [this]() {
        doAction(PAUSE_KEY);
    });
    /*media stop*/
    QAction *mediaStop= new QAction(this);
    mediaStop->setObjectName(QStringLiteral("Stop playback"));
    mediaStop->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(mediaStop, QList<QKeySequence>{Qt::Key_MediaStop});
    KGlobalAccel::self()->setShortcut(mediaStop, QList<QKeySequence>{Qt::Key_MediaStop});
    connect(mediaStop, &QAction::triggered, this, [this]() {
        USD_LOG(LOG_DEBUG,"stop_key...");
        doAction(STOP_KEY);
    });
    /*media preious*/
    QAction *mediaPre= new QAction(this);
    mediaPre->setObjectName(QStringLiteral("Previous track"));
    mediaPre->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(mediaPre, QList<QKeySequence>{Qt::Key_MediaPrevious});
    KGlobalAccel::self()->setShortcut(mediaPre, QList<QKeySequence>{Qt::Key_MediaPrevious});
    connect(mediaPre, &QAction::triggered, this, [this]() {
        USD_LOG(LOG_DEBUG,"PREVIOUS_KEY...");
        doAction(PREVIOUS_KEY);
    });
    /*media next*/
    QAction *mediaNext= new QAction(this);
    mediaNext->setObjectName(QStringLiteral("Next track"));
    mediaNext->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(mediaNext, QList<QKeySequence>{Qt::Key_MediaNext});
    KGlobalAccel::self()->setShortcut(mediaNext, QList<QKeySequence>{Qt::Key_MediaNext});
    connect(mediaNext, &QAction::triggered, this, [this]() {
        doAction(NEXT_KEY);
        USD_LOG(LOG_DEBUG,"NEXT_KEY...");
    });
    /*audio Rewind*/
    QAction *audioRewind= new QAction(this);
    audioRewind->setObjectName(QStringLiteral("Audio Rewind"));
    audioRewind->setProperty("componentName",QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(audioRewind, QList<QKeySequence>{Qt::Key_AudioRewind});
    KGlobalAccel::self()->setShortcut(audioRewind, QList<QKeySequence>{Qt::Key_AudioRewind});
    connect(audioRewind, &QAction::triggered, this, [this]() {
        doAction(REWIND_KEY);
    });
    /*audio Forward*/
    QAction *audioForward= new QAction(this);
    audioForward->setObjectName(QStringLiteral("Audio Forward"));
    audioForward->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(audioForward, QList<QKeySequence>{Qt::Key_AudioForward});
    KGlobalAccel::self()->setShortcut(audioForward, QList<QKeySequence>{Qt::Key_AudioForward});
    connect(audioForward, &QAction::triggered, this, [this]() {
        doAction(FORWARD_KEY);
    });
    /*audio Repeat*/
    QAction *audioRepeat= new QAction(this);
    audioRepeat->setObjectName(QStringLiteral("Audio Repeat"));
    audioRepeat->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(audioRepeat, QList<QKeySequence>{Qt::Key_AudioRepeat});
    KGlobalAccel::self()->setShortcut(audioRepeat, QList<QKeySequence>{Qt::Key_AudioRepeat});
    connect(audioRepeat, &QAction::triggered, this, [this]() {
        doAction(REPEAT_KEY);
    });

    /*audio random*/
    QAction *audioRandom= new QAction(this);
    audioRandom->setObjectName(QStringLiteral("Audio Random"));
    audioRandom->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(audioRandom, QList<QKeySequence>{Qt::Key_AudioRandomPlay});
    KGlobalAccel::self()->setShortcut(audioRandom, QList<QKeySequence>{Qt::Key_AudioRandomPlay});
    connect(audioRandom, &QAction::triggered, this, [this]() {
        doAction(RANDOM_KEY);
    });
    /*TODO magnifier*/
    QAction *magnifier= new QAction(this);
    magnifier->setObjectName(QStringLiteral("Toggle Magnifier"));
    magnifier->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(magnifier, QList<QKeySequence>{});
    KGlobalAccel::self()->setShortcut(magnifier, QList<QKeySequence>{});
    connect(magnifier, &QAction::triggered, this, [this]() {
        doAction(MAGNIFIER_KEY);
    });
    /*TODO screen reader*/
    QAction *screenReader= new QAction(this);
    screenReader->setObjectName(QStringLiteral("Toggle Screen Reader"));
    screenReader->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(screenReader, QList<QKeySequence>{});
    KGlobalAccel::self()->setShortcut(screenReader, QList<QKeySequence>{});
    connect(screenReader, &QAction::triggered, this, [this]() {
        doAction(SCREENREADER_KEY);
    });
    /*TODO on-screen keyboard*/
    QAction *onScreenKeyboard= new QAction(this);
    onScreenKeyboard->setObjectName(QStringLiteral("Toggle On-screen Keyboard"));
    onScreenKeyboard->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(onScreenKeyboard, QList<QKeySequence>{});
    KGlobalAccel::self()->setShortcut(onScreenKeyboard, QList<QKeySequence>{});
    connect(onScreenKeyboard, &QAction::triggered, this, [this]() {
        doAction(ON_SCREEN_KEYBOARD_KEY);
    });
    /*logout*/
    QAction *logout= new QAction(this);
    logout->setObjectName(QStringLiteral("Open shutdown interface"));
    logout->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(logout, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_Delete});
    KGlobalAccel::self()->setShortcut(logout, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_Delete });
    connect(logout, &QAction::triggered, this, [this]() {
        doAction(LOGOUT_KEY);
    });

    QAction *logout1= new QAction(this);
    logout1->setObjectName(QStringLiteral("Open shutdown Interface "));
    logout1->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(logout1, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_Period});
    KGlobalAccel::self()->setShortcut(logout1, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_Period });
    connect(logout1, &QAction::triggered, this, [this]() {
        doAction(LOGOUT_KEY);
    });

    //sideBar
    QAction *sideBar= new QAction(this);
    sideBar->setObjectName(QStringLiteral("Open sideBar "));
    sideBar->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(sideBar, QList<QKeySequence>{Qt::META + Qt::Key_A});
    KGlobalAccel::self()->setShortcut(sideBar, QList<QKeySequence>{Qt::META + Qt::Key_A});
    connect(sideBar, &QAction::triggered, this, [this]() {
        doAction(UKUI_SIDEBAR);
    });

    QAction *logout2 = new QAction(this);
    logout2->setObjectName(QStringLiteral("open shutdown interface"));
    logout2->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(logout2, QList<QKeySequence>{Qt::Key_PowerOff});
    KGlobalAccel::self()->setShortcut(logout2, QList<QKeySequence>{Qt::Key_PowerOff});

    connect(logout2, &QAction::triggered, this, [this]() {
        doPowerOffAction();
    });


    /*terminal2*/
    QAction *terminal2= new QAction(this);
    terminal2->setObjectName(QStringLiteral("Open Terminal"));
    terminal2->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(terminal2, QList<QKeySequence>{Qt::META + Qt::Key_T});
    KGlobalAccel::self()->setShortcut(terminal2, QList<QKeySequence>{Qt::META + Qt::Key_T});
    connect(terminal2, &QAction::triggered, this, [this]() {
        doAction(TERMINAL_KEY);
    });


    /*window switch*/
    QAction *wSwitch= new QAction(this);
    wSwitch->setObjectName(QStringLiteral("open windows switch"));
    wSwitch->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(wSwitch, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_W});
    KGlobalAccel::self()->setShortcut(wSwitch, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_W});
    connect(wSwitch, &QAction::triggered, this, [this]() {
        doAction(WINDOWSWITCH_KEY);
    });
    /*window switch2*/
    QAction *wSwitch2= new QAction(this);
    wSwitch2->setObjectName(QStringLiteral("Open Windows switch"));
    wSwitch2->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(wSwitch2, QList<QKeySequence>{Qt::META + Qt::Key_W});
    KGlobalAccel::self()->setShortcut(wSwitch2, QList<QKeySequence>{Qt::META + Qt::Key_W});
    connect(wSwitch2, &QAction::triggered, this, [this]() {
        doAction(WINDOWSWITCH_KEY_2);
    });
    /*system monitor*/
    QAction *monitor= new QAction(this);
    monitor->setObjectName(QStringLiteral("Open the system monitor"));
    monitor->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(monitor, QList<QKeySequence>{Qt::CTRL + Qt::SHIFT + Qt::Key_Escape});
    KGlobalAccel::self()->setShortcut(monitor, QList<QKeySequence>{Qt::CTRL + Qt::SHIFT + Qt::Key_Escape});
    connect(monitor, &QAction::triggered, this, [this]() {
        doAction(SYSTEM_MONITOR_KEY);
    });
    /*internal edit*/
    QAction *editor= new QAction(this);
    editor->setObjectName(QStringLiteral("Open the internet connectionr"));
    editor->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(editor, QList<QKeySequence>{Qt::META + Qt::Key_K});
    KGlobalAccel::self()->setShortcut(editor, QList<QKeySequence>{Qt::META + Qt::Key_K});
    connect(editor, &QAction::triggered, this, [this]() {
        doAction(CONNECTION_EDITOR_KEY);
    });
    /*ukui search*/
    QAction *ukuiSearch= new QAction(this);
    ukuiSearch->setObjectName(QStringLiteral("Open UKUI Search"));
    ukuiSearch->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(ukuiSearch, QList<QKeySequence>{Qt::META + Qt::Key_S});
    KGlobalAccel::self()->setShortcut(ukuiSearch, QList<QKeySequence>{Qt::META + Qt::Key_S});
    connect(ukuiSearch, &QAction::triggered, this, [this]() {
        doAction(GLOBAL_SEARCH_KEY);
    });
    /*kylin display switch*/
    QAction *dSwitch= new QAction(this);
    dSwitch->setObjectName(QStringLiteral("Open kylin display switch"));
    dSwitch->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(dSwitch, QList<QKeySequence>{Qt::META + Qt::Key_P});
    KGlobalAccel::self()->setShortcut(dSwitch, QList<QKeySequence>{Qt::META + Qt::Key_P});
    connect(dSwitch, &QAction::triggered, this, [this]() {

        if (UsdBaseClass::isTablet()){
            if (getCurrentMode()) {
                return;
            }
        }


        static QTime startTime = QTime::currentTime();
        int elapsed = 0;

        elapsed = startTime.msecsTo(QTime::currentTime());
        if (elapsed>0 && elapsed<1200){//避免过快刷屏,必须大于，1200ms执行一次,
            if (false == CheckProcessAlive("kydisplayswitch")){
                return;
            }
        }
        startTime = QTime::currentTime();
        doAction(KDS_KEY);

    });
    /*kylin display switch2*/
    QAction *dSwitch2= new QAction(this);
    dSwitch2->setObjectName(QStringLiteral("open kylin display switch"));
    dSwitch2->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(dSwitch2, QList<QKeySequence>{Qt::Key_Display});
    KGlobalAccel::self()->setShortcut(dSwitch2, QList<QKeySequence>{Qt::Key_Display});
    connect(dSwitch2, &QAction::triggered, this, [this]() {
        doAction(KDS_KEY2);
    });
    /*kylin eyeCare center*/
    QAction *eyeCare= new QAction(this);
    eyeCare->setObjectName(QStringLiteral("open kylin eyeCare center"));
    eyeCare->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(eyeCare, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_P});
    KGlobalAccel::self()->setShortcut(eyeCare, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_P});
    connect(eyeCare, &QAction::triggered, this, [this]() {
        doAction(UKUI_EYECARE_CENTER);
    });

    //just for test globalshut
//    QAction *sjhTest= new QAction(this);
//    sjhTest->setObjectName(QStringLiteral("sjh test"));
//    sjhTest->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
//    KGlobalAccel::self()->setDefaultShortcut(sjhTest, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_R});
//    KGlobalAccel::self()->setShortcut(sjhTest, QList<QKeySequence>{Qt::CTRL + Qt::ALT + Qt::Key_R});
//    connect(sjhTest, &QAction::triggered, this, [this]() {
//        USD_LOG(LOG_DEBUG,".");
//        sjhKeyTest();
//    });

    /*TODO Ukui Sidebar*/
//    QAction *sideBar= new QAction(this);
//    sideBar->setObjectName(QStringLiteral("Open ukui sidebar"));
//    sideBar->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
//    KGlobalAccel::self()->setDefaultShortcut(sideBar, QList<QKeySequence>{});
//    KGlobalAccel::self()->setShortcut(sideBar, QList<QKeySequence>{});
//    connect(sideBar, &QAction::triggered, this, [this]() {
//        //doAction();
//    });
}

void MediaKeysManager::mediaKeysStop()
{
    QList<GdkScreen*>::iterator l,end;
    bool needFlush;
    int i;

    USD_LOG(LOG_DEBUG, "Stooping media keys manager!");

//    XEventMonitor::instance()->exit();

    /*
    gdk_window_remove_filter(gdk_screen_get_root_window(gdk_screen_get_default()),
                             (GdkFilterFunc)acmeFilterEvents,
                             NULL);

    needFlush = false;
    gdk_x11_display_error_trap_push(gdk_display_get_default());
    for(i = 0; i < HANDLED_KEYS; ++i){
        if(keys[i].key){
            needFlush = true;
            grab_key_unsafe(keys[i].key,false,nullptr);
            g_free(keys[i].key->keycodes);
            g_free(keys[i].key);
            keys[i].key = NULL;
        }
    }

    if(needFlush)
        gdk_display_flush(gdk_display_get_default());

    gdk_x11_display_error_trap_pop_ignored(gdk_display_get_default());
    */
//    if(mStream != NULL)
//        g_clear_object(&mStream);
//    if(mControl != NULL)
//        g_clear_object(&mControl);
//    if(mInputControl != NULL)
//        g_clear_object(&mInputControl);
//    if(mInputStream != NULL)
//        g_clear_object(&mInputStream);
//    if(mContext != NULL)
//        g_clear_object(&mContext);
}

void MediaKeysManager::XkbEventsRelease(const QString &keyStr)
{
    QString KeyName;
    static bool ctrlFlag = false;

    /*
    if (keyStr.compare("Shift_L+Print") == 0 ||
        keyStr.compare("Shift_R+Print") == 0 ){
        executeCommand("kylin-screenshot", " gui");
        return;
    }

    if (keyStr.compare("Print") == 0){
        executeCommand("kylin-screenshot", " full");
        return;
    }

    if(keyStr.compare("Control_L+Shift_L+Escape") == 0 ||
       keyStr.compare("Shift_L+Control_L+Escape") == 0) {
          executeCommand("ukui-system-monitor", nullptr);
          return;
    }
    */

    if (keyStr.length() >= 10)
        KeyName = keyStr.left(10);

    if(KeyName.compare("Control_L+") == 0 ||
       KeyName.compare("Control_R+") == 0 )
        ctrlFlag = true;

    if((ctrlFlag && keyStr.compare("Control_L") == 0 )||
       (ctrlFlag && keyStr.compare("Control_R") == 0 )){
        ctrlFlag = false;
        return;
    } else if((m_ctrlFlag && keyStr.compare("Control_L") == 0 )||
              (m_ctrlFlag && keyStr.compare("Control_R") == 0 ))
            return;

    if (keyStr.compare("Control_L") == 0 ||
            keyStr.compare("Control_R") == 0) {
        if (pointSettings) {
            try  {
               QStringList QGsettingskeys = pointSettings->keys();
               if (QGsettingskeys.contains("locate-pointer")){
                pointSettings->set("locate-pointer", !pointSettings->get(POINTER_KEY).toBool());
               }
               else {
                   USD_LOG(LOG_DEBUG,"schema contins key...");
               }
            }
            catch(char *msg){

            }
        }
    }
}

void MediaKeysManager::XkbEventsPress(const QString &keyStr)
{
    QString KeyName;

    if (keyStr.length() >= 10)
        KeyName = keyStr.left(10);

    if(KeyName.compare("Control_L+") == 0 ||
       KeyName.compare("Control_R+") == 0 )
        m_ctrlFlag = true;

    if(m_ctrlFlag && keyStr.compare("Control_L") == 0 ||
       m_ctrlFlag && keyStr.compare("Control_R") == 0 ){
        m_ctrlFlag = false;
        return;
    }
}



void MediaKeysManager::MMhandleRecordEvent(xEvent* data)
{
    if(UsdBaseClass::isUseXEventAsShutKey()) {
        Display* display;
        guint eventKeysym;

        display =  QX11Info::display();

        xEvent * event = (xEvent *)data;
        eventKeysym =XkbKeycodeToKeysym(display, event->u.u.detail, 0, 0);

        if (eventKeysym == XKB_KEY_XF86AudioMute) {
           xEventHandle(MUTE_KEY, event);

        } else if (eventKeysym == XKB_KEY_XF86AudioLowerVolume) {
            doAction(VOLUME_DOWN_KEY);

        } else if (eventKeysym == XKB_KEY_XF86AudioRaiseVolume) {
            doAction(VOLUME_UP_KEY);

        } else if (eventKeysym == XKB_KEY_XF86MonBrightnessDown) {
            xEventHandle(BRIGHT_DOWN_KEY, event);

        } else if (eventKeysym == XKB_KEY_XF86MonBrightnessUp) {
            xEventHandle(BRIGHT_UP_KEY, event);

        } else if (eventKeysym == XKB_KEY_Print && mXEventMonitor->getShiftPressStatus() && mXEventMonitor->getCtrlPressStatus()) {
            xEventHandle(AREA_SCREENSHOT_KEY, event);

        } else if (eventKeysym == XKB_KEY_Print && mXEventMonitor->getCtrlPressStatus()) {
            xEventHandle(WINDOW_SCREENSHOT_KEY, event);

        } else if (eventKeysym == XKB_KEY_Print) {
            xEventHandle(SCREENSHOT_KEY, event);

        } else if (eventKeysym == XKB_KEY_XF86RFKill) {
            xEventHandle(RFKILL_KEY, event);

        } else if(eventKeysym == XKB_KEY_XF86WLAN) {
            xEventHandle(WLAN_KEY, event);

        } else if (eventKeysym == XKB_KEY_XF86TouchpadToggle) {
            xEventHandle(TOUCHPAD_KEY, event);

        } else if (eventKeysym == XKB_KEY_XF86AudioMicMute) {
            xEventHandle(MIC_MUTE_KEY, event);

        } else if (eventKeysym == XKB_KEY_XF86TouchpadOn) {
            xEventHandle(TOUCHPAD_ON_KEY, event);

        } else if (eventKeysym == XKB_KEY_XF86TouchpadOff) {
            xEventHandle(TOUCHPAD_OFF_KEY, event);

        } else if(true == mXEventMonitor->getCtrlPressStatus()) {
            if (pointSettings) {
                QStringList QGsettingskeys = pointSettings->keys();
                if (QGsettingskeys.contains("locate-pointer")){
                    pointSettings->set("locate-pointer", !pointSettings->get(POINTER_KEY).toBool());
                }
            }
        }
    }
}

void MediaKeysManager::MMhandleRecordEventRelease(xEvent* data)
{
    if(UsdBaseClass::isUseXEventAsShutKey()) {
        Display* display;
        guint eventKeysym;
        display =  QX11Info::display();

        xEvent * event = (xEvent *)data;
        eventKeysym =XkbKeycodeToKeysym(display, event->u.u.detail, 0, 0);

        if (eventKeysym == XKB_KEY_XF86AudioMute) {
           xEventHandleRelease(MUTE_KEY);
        }  else if (eventKeysym == XKB_KEY_XF86MonBrightnessDown) {
            xEventHandleRelease(BRIGHT_DOWN_KEY);

        } else if (eventKeysym == XKB_KEY_XF86MonBrightnessUp) {
            xEventHandleRelease(BRIGHT_UP_KEY);

        } else if (eventKeysym == XKB_KEY_Print && mXEventMonitor->getShiftPressStatus() && mXEventMonitor->getCtrlPressStatus()) {
            xEventHandleRelease(AREA_SCREENSHOT_KEY);

        } else if (eventKeysym == XKB_KEY_Print && mXEventMonitor->getCtrlPressStatus()) {
            xEventHandleRelease(WINDOW_SCREENSHOT_KEY);

        } else if (eventKeysym == XKB_KEY_Print) {
            xEventHandleRelease(SCREENSHOT_KEY);

        } else if(eventKeysym == XKB_KEY_XF86RFKill) {
            xEventHandleRelease(RFKILL_KEY);

        } else if (eventKeysym == XKB_KEY_XF86WLAN) {
            xEventHandleRelease(WLAN_KEY);

        }else if (eventKeysym == XKB_KEY_XF86TouchpadToggle) {
            xEventHandleRelease(TOUCHPAD_KEY);

        } else if (eventKeysym == XKB_KEY_XF86AudioMicMute) {
            xEventHandleRelease(MIC_MUTE_KEY);

        } else if (eventKeysym == XKB_KEY_XF86TouchpadOn) {
            xEventHandleRelease(TOUCHPAD_ON_KEY);

        } else if (eventKeysym == XKB_KEY_XF86TouchpadOff) {
            xEventHandleRelease(TOUCHPAD_OFF_KEY);

        }
    }
}

void MediaKeysManager::initXeventMonitor()
{
//   XEventMonitor::instance()->start();

//   connect(XEventMonitor::instance(), SIGNAL(keyRelease(QString)),this, SLOT(XkbEventsRelease(QString)));
//    connect(XEventMonitor::instance(), SIGNAL(keyPress(QString)),this, SLOT(XkbEventsPress(QString)));


    connect(mXEventMonitor, SIGNAL(keyPress(xEvent*)), this, SLOT(MMhandleRecordEvent(xEvent*)), Qt::QueuedConnection);
    connect(mXEventMonitor, SIGNAL(keyRelease(xEvent*)), this, SLOT(MMhandleRecordEventRelease(xEvent*)), Qt::QueuedConnection);
}

void MediaKeysManager::initScreens()
{
    GdkDisplay *display;
    GdkScreen  *screen;

    display = gdk_display_get_default();
    screen = gdk_display_get_default_screen(display);
    mCurrentScreen = screen;
}

GdkFilterReturn
MediaKeysManager::acmeFilterEvents(GdkXEvent* xevent,GdkEvent* event,void* data)
{
    XEvent    *xev = (XEvent *) xevent;
    XAnyEvent *xany = (XAnyEvent *) xevent;
    int       i;

    /* verify we have a key event */
    if (xev->type != KeyPress && xev->type != KeyRelease)
        return GDK_FILTER_CONTINUE;

    for (i = 0; i < HANDLED_KEYS; i++) {
        if (match_key (keys[i].key, xev)) {
            switch (keys[i].key_type) {
            case VOLUME_DOWN_KEY:
            case VOLUME_UP_KEY:
                /* auto-repeatable keys */
                if (xev->type != KeyPress)
                    return GDK_FILTER_CONTINUE;
                break;
            default:
                if (xev->type != KeyRelease)
                    return GDK_FILTER_CONTINUE;
            }

            mManager->mCurrentScreen = mManager->acmeGetScreenFromEvent(xany);

            if (mManager->doAction(keys[i].key_type) == false)
                return GDK_FILTER_REMOVE;
            else
                return GDK_FILTER_CONTINUE;
        }
    }

    return GDK_FILTER_CONTINUE;
}
#if 0 //delete libmatemixer

void MediaKeysManager::onContextStateNotify(MateMixerContext *context,
                                            GParamSpec       *pspec,
                                            MediaKeysManager *manager)
{
    updateDefaultInput(manager);
    updateDefaultOutput(manager);
}

void MediaKeysManager::onContextDefaultInputNotify(MateMixerContext *context,
                                                   GParamSpec       *pspec,
                                                   MediaKeysManager *manager)
{
    updateDefaultInput(manager);
}

void MediaKeysManager::onContextDefaultOutputNotify(MateMixerContext *context,
                                                    GParamSpec       *pspec,
                                                    MediaKeysManager *manager)
{
    updateDefaultOutput(manager);
}

void MediaKeysManager::onContextStreamRemoved(MateMixerContext *context,
                                              char             *name,
                                              MediaKeysManager *mManager)
{
    if (mManager->mStream != NULL) {
        MateMixerStream *stream =
            mate_mixer_context_get_stream (mManager->mContext, name);

        if (stream == mManager->mStream) {
            if(mManager->mStream  && mManager->mControl){
                g_clear_object (&mManager->mStream);
                g_clear_object (&mManager->mControl);
            }
        }
    }
}

void MediaKeysManager::updateDefaultInput(MediaKeysManager *mManager)
{
    MateMixerStream        *inputStream;
    MateMixerStreamControl *inputControl = NULL;

    inputStream = mate_mixer_context_get_default_input_stream (mManager->mContext);
    if (inputStream != NULL)
        inputControl = mate_mixer_stream_get_default_control (inputStream);

    if(inputStream == mManager->mInputStream)
        return;

    if(mManager->mInputStream && mManager->mInputControl){
        g_clear_object (&mManager->mInputStream);
        g_clear_object (&mManager->mInputControl);
    }

    if (inputControl != NULL) {
             MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags (inputControl);

            /* Do not use the stream if it is not possible to mute it or
             * change the volume */
            if (!(flags & MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE) &&
                !(flags & MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE))
                    return;

            mManager->mInputStream = (MateMixerStream *)g_object_ref(inputStream);
            mManager->mInputControl = (MateMixerStreamControl *)g_object_ref(inputControl);
            USD_LOG(LOG_DEBUG, "Default input stream updated to %s",
                     mate_mixer_stream_get_name (inputStream));
    } else
            USD_LOG(LOG_DEBUG, "Default input stream unset");
}

void MediaKeysManager::updateDefaultOutput(MediaKeysManager *mManager)
{
    MateMixerStream        *stream;
    MateMixerStreamControl *control = NULL;


    stream = mate_mixer_context_get_default_output_stream (mManager->mContext);
    if (stream != NULL)
        control = mate_mixer_stream_get_default_control (stream);

    if (stream == mManager->mStream)
           return;

    if(mManager->mStream  && mManager->mControl){
        g_clear_object (&mManager->mStream);
        g_clear_object (&mManager->mControl);
    }
    if (control != NULL) {
            MateMixerStreamControlFlags flags = mate_mixer_stream_control_get_flags (control);

           /* Do not use the stream if it is not possible to mute it or
            * change the volume */
           if (!(flags & MATE_MIXER_STREAM_CONTROL_MUTE_WRITABLE) &&
               !(flags & MATE_MIXER_STREAM_CONTROL_VOLUME_WRITABLE))
                   return;

           mManager->mStream = (MateMixerStream *)g_object_ref(stream);
           mManager->mControl = (MateMixerStreamControl *)g_object_ref(control);
           USD_LOG(LOG_DEBUG, "Default output stream updated to %s",
                    mate_mixer_stream_get_name (stream));
   } else
           USD_LOG(LOG_DEBUG, "Default output stream unset");
    g_signal_connect ( G_OBJECT (mManager->mControl),
                       "notify::volume",
                       G_CALLBACK (onStreamControlVolumeNotify),
                       mManager);
    g_signal_connect ( G_OBJECT (mManager->mControl),
                       "notify::mute",
                       G_CALLBACK (onStreamControlMuteNotify),
                       mManager);
}


/*!
 * \brief
 * \details
 * 音量值更改
 */
void MediaKeysManager::onStreamControlVolumeNotify (MateMixerStreamControl *control,GParamSpec *pspec,MediaKeysManager *mManager)
{
#if 0
    MateMixerStream *stream = mate_mixer_stream_control_get_stream(control);
    USD_LOG(LOG_DEBUG, "onStreamControlVolumeNotify control name: %s volume: %d",
          mate_mixer_stream_control_get_name(control) ,  mate_mixer_stream_control_get_volume (control));
    if(!MATE_MIXER_IS_STREAM(stream)){
        USD_LOG(LOG_DEBUG,"Add exception handling ---------");
        stream = mate_mixer_context_get_stream(mManager->mContext,mate_mixer_stream_control_get_name(control));
        //使用命令重新设置音量
        int volume = mate_mixer_stream_control_get_volume(control);
        QString cmd = "pactl set-sink-volume "+ QString(mate_mixer_stream_control_get_name(control)) +" "+ QString::number(volume,10);
        system(cmd.toLocal8Bit().data());
    }
#endif
}

/*!
 * \brief
 * \details
 * 静音通知
 */
void MediaKeysManager::onStreamControlMuteNotify (MateMixerStreamControl *control,GParamSpec *pspec,MediaKeysManager *mManager)
{
#if 0
    MateMixerStream *stream = mate_mixer_stream_control_get_stream(control);
    USD_LOG(LOG_DEBUG, "onStreamControlMuteNotify control name: %s volume: %d",
          mate_mixer_stream_control_get_name(control) ,  mate_mixer_stream_control_get_mute (control));
    if(!MATE_MIXER_IS_STREAM(stream)){
        USD_LOG(LOG_DEBUG,"Add exception handling ---------");
        stream = mate_mixer_context_get_stream(mManager->mContext,mate_mixer_stream_control_get_name(control));
        //使用命令重新设置音量
        bool isMuted = mate_mixer_stream_control_get_mute(control);
        QString cmd = "pactl set-sink-mute "+ QString(mate_mixer_stream_control_get_name(control)) +" "+ QString::number(isMuted,10);
        system(cmd.toLocal8Bit().data());
    }
#endif
}
#endif  //delete matemixer
GdkScreen *
MediaKeysManager::acmeGetScreenFromEvent (XAnyEvent *xanyev)
{
    GdkWindow *window;

    window = gdk_screen_get_root_window (gdk_screen_get_default());

    if (GDK_WINDOW_XID (window) == xanyev->window)
        return gdk_screen_get_default();

    return NULL;
}

bool MediaKeysManager::doAction(int type)
{
    static QTime startTime = QTime::currentTime();

    int elapsed = 0;
    static uint lastKeySym = 0x00;

//    if ((getScreenLockState()) && (type!=MUTE_KEY && type!=VOLUME_DOWN_KEY && type!=VOLUME_UP_KEY)) {
//        USD_LOG(LOG_DEBUG,"can;t use it..");
//        return false;
//    }

    //TODO:需要考虑清楚如何过滤掉重复事件，
//    if (lastKeySym == type){//考虑到一个应用针对多个快捷键，所以不能以按键值进行次数区分必须以序号进行区分，否则第二个以后的快捷键不生效
//        elapsed = startTime.msecsTo(QTime::currentTime());

//        if (elapsed>=0 && elapsed<50){//避免过快刷屏,必须大于，50ms执行一次,
//            return false;    //goto FREE_DISPLAY;
//        }
//    }

    startTime = QTime::currentTime();
    lastKeySym = type;


    switch(type){
    case TOUCHPAD_KEY:
        doTouchpadAction(STATE_TOGGLE);
        break;
    case TOUCHPAD_ON_KEY:
        doTouchpadAction(STATE_ON);
        break;
    case TOUCHPAD_OFF_KEY:
        doTouchpadAction(STATE_OFF);
        break;
    case MUTE_KEY:
    case VOLUME_DOWN_KEY:
    case VOLUME_UP_KEY:
        doSoundActionALSA(type);
        break;
    case MIC_MUTE_KEY:
        doMicSoundAction();
        break;
    case BRIGHT_UP_KEY:
    case BRIGHT_DOWN_KEY:
        doBrightAction(type);
        break;
    case POWER_KEY:
        doShutdownAction();
        break;
    case LOGOUT_KEY:
        doLogoutAction();
        break;
    case EJECT_KEY:
        break;
    case HOME_KEY:
        doOpenHomeDirAction();
        break;
    case SEARCH_KEY:
        doSearchAction();
        break;
    case EMAIL_KEY:
        doUrlAction("mailto");
        break;
    case SCREENSAVER_KEY:
    case SCREENSAVER_KEY_2:
        doScreensaverAction();
        break;
    case SETTINGS_KEY:
    case SETTINGS_KEY_2:
        doSettingsAction();
        break;
    case WINDOWSWITCH_KEY:
    case WINDOWSWITCH_KEY_2:
        doWindowSwitchAction();
        break;
    case FILE_MANAGER_KEY:
    case FILE_MANAGER_KEY_2:
        doOpenFileManagerAction();
        break;
    case HELP_KEY:
        doUrlAction("help");
        break;
    case WWW_KEY:
        doUrlAction("http");
        break;
    case MEDIA_KEY:
        doMediaAction();
        break;
    case CALCULATOR_KEY:
        doOpenCalcAction();
        break;
    case PLAY_KEY:
        doMultiMediaPlayerAction("Play");
        break;
    case PAUSE_KEY:
        doMultiMediaPlayerAction("Pause");
        break;
    case STOP_KEY:
        doMultiMediaPlayerAction("Stop");
        break;
    case PREVIOUS_KEY:
        doMultiMediaPlayerAction("Previous");
        break;
    case NEXT_KEY:
        doMultiMediaPlayerAction("Next");
        break;
    case REWIND_KEY:
        doMultiMediaPlayerAction("Rewind");
        break;
    case FORWARD_KEY:
        doMultiMediaPlayerAction("FastForward");
        break;
    case REPEAT_KEY:
        doMultiMediaPlayerAction("Repeat");
        break;
    case RANDOM_KEY:
        doMultiMediaPlayerAction("Shuffle");
        break;
    case MAGNIFIER_KEY:
        doMagnifierAction();
        break;
    case SCREENREADER_KEY:
        doScreensaverAction();
        break;
    case ON_SCREEN_KEYBOARD_KEY:
        doOnScreenKeyboardAction();
        break;
    case TERMINAL_KEY:
    case TERMINAL_KEY_2:
        doOpenTerminalAction();
        break;
    case SCREENSHOT_KEY:
        doScreenshotAction(" full");
        break;
    case AREA_SCREENSHOT_KEY:
        doScreenshotAction(" gui");
        break;
    case WINDOW_SCREENSHOT_KEY:
        doScreenshotAction(" screen");
        break;
    case SYSTEM_MONITOR_KEY:
        doOpenMonitor();
        break;
    case CONNECTION_EDITOR_KEY:
        doOpenConnectionEditor();
        break;
    case GLOBAL_SEARCH_KEY:
        doOpenUkuiSearchAction();
        break;
    case KDS_KEY:
    case KDS_KEY2:
        doOpenKdsAction();
        break;
    case WLAN_KEY:
        doWlanAction();
        break;
    case WEBCAM_KEY:
        doWebcamAction();
        break;
    case UKUI_SIDEBAR:
        doSidebarAction();
        break;
    case UKUI_EYECARE_CENTER:
        doEyeCenterAction();
        break;
    case RFKILL_KEY:
        doFlightModeAction();
        break;
    default:
        break;
    }
    return false;
}

bool isValidShortcut (const QString& string)
{
    if (string.isNull() || string.isEmpty())
        return false;
    if (string == "disabled")
        return false;

    return true;
}

void MediaKeysManager::initKbd()
{
    int i;
    bool needFlush = false;

//    gdk_x11_display_error_trap_push(gdk_display_get_default());
    QObject::connect(mSettings, &QGSettings::changed,
            this, &MediaKeysManager::updateKbdCallback);

    connect(mSettings, SIGNAL(changed(QString)), this, SLOT(updateKbdCallback(QString)));


    for(i = 0; i < HANDLED_KEYS; ++i){
        QString tmp,schmeasKey;
        Key* key;

        if(NULL != keys[i].settings_key){
            schmeasKey = keys[i].settings_key;
            tmp = mSettings->get(schmeasKey).toString();
        }else
            tmp = keys[i].hard_coded;

        if(!isValidShortcut(tmp)){
            tmp.clear();
            continue;
        }

        key = g_new0(Key,1);
        if(!egg_accelerator_parse_virtual(tmp.toLatin1().data(),&key->keysym,&key->keycodes,
                                          (EggVirtualModifierType*)&key->state)){
            tmp.clear();
            g_free(key);
            continue;
        }

        tmp.clear();
        keys[i].key = key;
        needFlush = true;
        grab_key_unsafe(key,true, nullptr);
    }

//    if(needFlush)
//        gdk_display_flush(gdk_display_get_default());
//    if(gdk_x11_display_error_trap_pop(gdk_display_get_default()))
//        qWarning("Grab failed for some keys,another application may already have access the them.");
}

void MediaKeysManager::updateKbdCallback(const QString &key)
{
    int i;
    bool needFlush = true;

    if(key.isNull())
        return;

    gdk_x11_display_error_trap_push (gdk_display_get_default());

    /* Find the key that was modified */
    for (i = 0; i < HANDLED_KEYS; i++) {
        if (0 == key.compare(keys[i].settings_key)) {
            QString tmp;
            Key  *key;

            if (NULL != keys[i].key) {
                needFlush = true;
                grab_key_unsafe (keys[i].key, false, nullptr);
            }

            g_free (keys[i].key);
            keys[i].key = NULL;

            /* We can't have a change in a hard-coded key */
            if(NULL != keys[i].settings_key){
                qWarning("settings key value is NULL,exit!");
                //return;
            }

            tmp = mSettings->get(keys[i].settings_key).toString();

            if (false == isValidShortcut(tmp)) {
                tmp.clear();
                break;
            }
            key = g_new0 (Key, 1);

            if (!egg_accelerator_parse_virtual (tmp.toLatin1().data(), &key->keysym, &key->keycodes,
                                                (EggVirtualModifierType*)&key->state)) {
                tmp.clear();
                g_free (key);
                break;
            }

            needFlush = true;
            grab_key_unsafe (key, true, nullptr);
            keys[i].key = key;

            tmp.clear();
            break;
        }
    }

    if (needFlush)
        gdk_display_flush (gdk_display_get_default());
    if (gdk_x11_display_error_trap_pop (gdk_display_get_default()))
        qWarning("Grab failed for some keys, another application may already have access the them.");
}

void MediaKeysManager::doTouchpadAction(int state)
{
    QGSettings *touchpadSettings;
    bool touchpadState;

    touchpadSettings = new QGSettings("org.ukui.peripherals-touchpad");
    touchpadState = touchpadSettings->get("touchpad-enabled").toBool();
    if(FALSE == touchpad_is_present()){
        mDeviceWindow->setAction("touchpad-disabled");
        return;
    }

    if (STATE_TOGGLE == state) {
        mDeviceWindow->setAction(!touchpadState ? "ukui-touchpad-on" : "ukui-touchpad-off");
        touchpadSettings->set("touchpad-enabled",!touchpadState);
    } else if (STATE_ON == state) {
        mDeviceWindow->setAction("ukui-touchpad-on");
        touchpadSettings->set("touchpad-enabled",STATE_ON);
    } else if (STATE_OFF == state) {
        mDeviceWindow->setAction("ukui-touchpad-off");
        touchpadSettings->set("touchpad-enabled",STATE_OFF);
    }
    mDeviceWindow->dialogShow();



    delete touchpadSettings;
}

void MediaKeysManager::doMicSoundAction()
{
    bool mute;
    mpulseAudioManager = new pulseAudioManager(this);

    mute = !mpulseAudioManager->getMicMute();
    mpulseAudioManager->setMicMute(mute);
    mDeviceWindow->setAction ( mute ? "ukui-microphone-off" : "ukui-microphone-on");
    mDeviceWindow->dialogShow();

    delete mpulseAudioManager;


}

void MediaKeysManager::doBrightAction(int type)
{
    int brightValue;
    QGSettings* settings = new QGSettings(GPM_SETTINGS_SCHEMA);
    switch (type){
    case BRIGHT_UP_KEY:
        brightValue = settings->get(GPM_SETTINGS_BRIGHTNESS_AC).toInt() + STEP_BRIGHTNESS;
        if(brightValue >= MAX_BRIGHTNESS){
            brightValue = MAX_BRIGHTNESS;
        }
        break;
    case BRIGHT_DOWN_KEY:
        brightValue = settings->get(GPM_SETTINGS_BRIGHTNESS_AC).toInt() - STEP_BRIGHTNESS;
        if(brightValue <= STEP_BRIGHTNESS){
            brightValue = STEP_BRIGHTNESS;
        }
        break;
    }
    settings->set(GPM_SETTINGS_BRIGHTNESS_AC,brightValue);
    mVolumeWindow->setBrightIcon("display-brightness-symbolic");
    mVolumeWindow->setBrightValue(brightValue);
    mVolumeWindow->dialogBrightShow();
    delete settings;
}

void MediaKeysManager::doSoundActionALSA(int keyType)
{
    int last_volume;
    bool lastmuted;
    bool soundChanged = false;

    mpulseAudioManager = new pulseAudioManager(this);

    int volumeStep = mSettings->get("volume-step").toInt();
    int volume  = mpulseAudioManager->getVolume();
    int muted  = mpulseAudioManager->getMute();
    USD_LOG(LOG_DEBUG,"getMute muted : %d",muted);

    int volumeCvStep = mpulseAudioManager->getStepVolume();
    int volumeMin = mpulseAudioManager->getMinVolume();
    int volumeMax = mpulseAudioManager->getMaxVolume();

    volumeStep*=volumeCvStep;

    last_volume = volume;
    lastmuted = muted;

    switch(keyType){
    case MUTE_KEY:
            muted = !muted;
        break;
    case VOLUME_DOWN_KEY:
        if(volume <= (volumeMin + volumeStep)){
            volume = volumeMin;
            muted = true;
        }else{
            volume -= volumeStep;
            muted = false;
        }

        if(volume <= volumeMin){
            volume = volumeMin;
            muted = true;
        }

        break;
    case VOLUME_UP_KEY:
        if(muted){
            muted = false;
        }

        volume += volumeStep;

        if (volume >= volumeMax) {
            volume = volumeMax;
        }

        break;
    }

    if (last_volume != volume) {
        soundChanged = true;
    }

    if (volumeMin == volume) {
        muted = true;
    }

    mpulseAudioManager->setVolume(volume);
    mVolumeWindow->setVolumeRange(volumeMin, volumeMax);
    mpulseAudioManager->setMute(muted);

    updateDialogForVolume(volume,muted,soundChanged);

    delete mpulseAudioManager;
}

void MediaKeysManager::doSoundAction(int keyType)
{
#if 0
    bool muted,mutedLast,soundChanged;  //是否静音，上一次值记录，是否改变
    int volume,volumeMin,volumeMax;    //当前音量值，最小音量值，最大音量值
    uint volumeStep,volumeLast;         //音量步长，上一次音量值

    if(NULL == mControl)
        return;

    volumeMin = mate_mixer_stream_control_get_min_volume(mControl);
    volumeMax = mate_mixer_stream_control_get_normal_volume(mControl);
    volumeStep = mSettings->get("volume-step").toInt();

    if(volumeStep <= 0 || volumeStep > 100)
        volumeStep = VOLUMESTEP;
    volumeStep = (volumeStep * volumeMax) / 100;

    volume = volumeLast = mate_mixer_stream_control_get_volume(mControl);
    muted = mutedLast = mate_mixer_stream_control_get_mute(mControl);
    USD_LOG(LOG_DEBUG,"volumeMin volume%d",volume);
    switch(keyType){
    case MUTE_KEY:
        // if(volume == volumeMin) //HW需求：音量为0时也支持F4快捷键静音和解除静音
        //     muted = true;
        // else
            muted = !muted;
        break;
    case VOLUME_DOWN_KEY:
        if(volume <= (volumeMin + volumeStep)){
            volume = volumeMin;
            muted = true;
            USD_LOG(LOG_DEBUG,"volumeMin volume%d",volume);
        }else{
            volume -= volumeStep;
            USD_LOG(LOG_DEBUG,"volumeMin volume%d",volume);
            muted = false;
        }
        if(volume < 300){
            volume = volumeMin;
            muted = true;
        }
        USD_LOG(LOG_DEBUG,"volumeMin volume%d",volume);
        break;
    case VOLUME_UP_KEY:
        if(muted){
            muted = false;
            if(volume <= (volumeMin + volumeStep))
                volume = volumeMin + volumeStep;
        }else
            volume = midValue(volume + volumeStep, volumeMin, volumeMax);
        break;
    }

    if(muted != mutedLast){
        if(mate_mixer_stream_control_set_mute(mControl, muted))
            soundChanged = true;
        else
            muted = mutedLast;
    }

    if(mate_mixer_stream_control_get_volume(mControl) != volume){
        if(mate_mixer_stream_control_set_volume(mControl,volume)) {
            soundChanged = true;
            USD_LOG(LOG_DEBUG,"setok/// volume:%d",volume);
        }
        else {
            volume = volumeLast;
            USD_LOG(LOG_DEBUG,"set fail.");
        }
    }
    USD_LOG(LOG_DEBUG,"volumeMin volume%d %d~%d get :%d",volume,volumeMin,volumeMax,mate_mixer_stream_control_get_volume(mControl) );
    mVolumeWindow->setVolumeRange(volumeMin, volumeMax);
    updateDialogForVolume(volume,muted,soundChanged);
#endif
//    usleep(1500);
}

void MediaKeysManager::updateDialogForVolume(uint volume,bool muted,bool soundChanged)
{
    int v = volume;
    USD_LOG(LOG_DEBUG, "volume = %d, muted is %d", v, muted);
    mVolumeWindow->setVolumeMuted(muted);
    mVolumeWindow->setVolumeLevel(volume);
    mVolumeWindow->dialogVolumeShow();
}

void processAbstractPath(QString& process)
{
    QString tmpPath;
    QFileInfo fileInfo;

    tmpPath = "/usr/bin/" + process;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists()){
        process = tmpPath;
        return;
    }

    tmpPath.clear();
    tmpPath = "/usr/sbin/" + process;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists()){
        process = tmpPath;
        return;
    }

    process = "";
}
bool binaryFileExists(const QString& binary)
{
    QString tmpPath;
    QFileInfo fileInfo;

    tmpPath = "/usr/bin/" + binary;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists())
        return true;

    tmpPath.clear();
    tmpPath = "/usr/sbin/" + binary;
    fileInfo.setFile(tmpPath);
    if(fileInfo.exists())
        return true;

    return false;
}

void MediaKeysManager::executeCommand(const QString& command,const QString& paramter){
    QString cmd = command + paramter;
    char   **argv;
    int     argc;
    bool    retval;

    //processAbstractPath(cmd);
    if(!cmd.isEmpty()){
        if (g_shell_parse_argv (cmd.toLatin1().data(), &argc, &argv, NULL)) {
            retval = g_spawn_async (g_get_home_dir (),
                                    argv,
                                    NULL,
                                    G_SPAWN_SEARCH_PATH,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
            g_strfreev (argv);
        }
        //mExecCmd->execute(cmd + paramter);//mExecCmd->start(cmd + paramter);
    }
    else
        qWarning("%s cannot found at system path!",command.toLatin1().data());
}

void MediaKeysManager::doShutdownAction()
{
    executeCommand("ukui-session-tools"," --shutdown");
}

void MediaKeysManager::doLogoutAction()
{
    executeCommand("ukui-session-tools","");
}

void MediaKeysManager::doPowerOffAction()
{
    if (true == UsdBaseClass::isTablet()) {
        doAction(SCREENSAVER_KEY_2);
    } else {
        static QTime startTime = QTime::currentTime();
        static int elapsed = -1;

        elapsed = startTime.msecsTo(QTime::currentTime());
        if(elapsed > 0 && elapsed <= TIME_LIMIT){
            return;
        }
        startTime = QTime::currentTime();

        power_state = powerSettings->getEnum(POWER_BUTTON_KEY);
        switch (power_state) {
            case POWER_HIBERNATE:
                executeCommand("ukui-session-tools"," --hibernate");
                break;
            case POWER_INTER_ACTIVE:
                doAction(LOGOUT_KEY);
                break;
            case POWER_SHUTDOWN:
                //doAction(POWER_KEY);
                executeCommand("ukui-session-tools"," --shutdown");
                break;
            case POWER_SUSPEND:
                executeCommand("ukui-session-tools"," --suspend");
                break;
        }
    }
}


void MediaKeysManager::doOpenHomeDirAction()
{
    QString homePath;

    homePath = QDir::homePath();
    executeCommand("peony"," --show-folders " + homePath);
}

void MediaKeysManager::doSearchAction()
{
    QString tool1,tool2,tool3;

    tool1 = "beagle-search";
    tool2 = "tracker-search-tool";
    tool3 = "mate-search-tool";

    if(binaryFileExists(tool1)){
        executeCommand(tool1,"");
    }else if(binaryFileExists(tool2)){
        executeCommand(tool2,"");
    }else
        executeCommand(tool3,"");
}

void MediaKeysManager::doScreensaverAction()
{
    QString tool1,tool2;

    tool1 = "ukui-screensaver-command";
    tool2 = "xscreensaver-command";

    if(binaryFileExists(tool1))
        executeCommand(tool1," --lock");
    else
        executeCommand(tool2," --lock");
}

void MediaKeysManager::doSettingsAction()
{
    executeCommand("ukui-control-center","");
}

void MediaKeysManager::doWindowSwitchAction()
{
    executeCommand("ukui-window-switch"," --show-workspace");
}

void MediaKeysManager::doOpenFileManagerAction()
{
    executeCommand("peony","");
}

void MediaKeysManager::doMediaAction()
{
}

void MediaKeysManager::doOpenCalcAction()
{
    QString tool1,tool2,tool3;

    tool1 = "galculator";
    tool2 = "mate-calc";
    tool3 = "gnome-calculator";

    if(binaryFileExists(tool1)){
        executeCommand(tool1,"");
    }else if(binaryFileExists(tool2)){
        executeCommand(tool2,"");
    }else
        executeCommand(tool3,"");
}

void MediaKeysManager::doToggleAccessibilityKey(const QString key)
{
    QGSettings* toggleSettings;
    bool state;

    toggleSettings = new QGSettings("org.gnome.desktop.a11y.applications");
    state = toggleSettings->get(key).toBool();
    toggleSettings->set(key,!state);

    delete toggleSettings;
}

void MediaKeysManager::doMagnifierAction()
{
    doToggleAccessibilityKey("screen-magnifier-enabled");
}

void MediaKeysManager::doScreenreaderAction()
{
    doToggleAccessibilityKey("screen-reader-enabled");
}

void MediaKeysManager::doOnScreenKeyboardAction()
{
    doToggleAccessibilityKey("screen-keyboard-enabled");
}

void MediaKeysManager::doOpenTerminalAction()
{
    executeCommand("mate-terminal","");
}

void MediaKeysManager::doScreenshotAction(const QString pramater)
{
    executeCommand("kylin-screenshot",pramater);
}

void MediaKeysManager::doSidebarAction()
{
    executeCommand("ukui-sidebar"," -show");
}

void MediaKeysManager::doOpenMonitor()
{
    executeCommand("ukui-system-monitor","");
}
void MediaKeysManager::doOpenConnectionEditor()
{
    executeCommand("nm-connection-editor","");
}

void MediaKeysManager::doOpenUkuiSearchAction()
{
    QDBusMessage message =
                QDBusMessage::createMethodCall("com.ukui.search.service",
                                               "/",
                                               "org.ukui.search.service",
                                               "showWindow");

        QDBusMessage response = QDBusConnection::sessionBus().call(message);

        if (response.type() != QDBusMessage::ReplyMessage){
            USD_LOG(LOG_DEBUG, "priScreenChanged called failed");
            executeCommand("ukui-search"," -s");
        }
}

void MediaKeysManager::doOpenKdsAction()
{
     executeCommand("ukydisplayswitch","");
}

void MediaKeysManager::doWlanAction()
{
    int wlanState = RfkillSwitch::instance()->getCurrentWlanMode();

    if(wlanState == -1)
    {
        return;
    }
    if(wlanState) {
        mDeviceWindow->setAction("ukui-wifi-on");
        RfkillSwitch::instance()->turnWifiOn();
    } else {
        mDeviceWindow->setAction("ukui-wifi-off");
    }
    mDeviceWindow->dialogShow();
}

void MediaKeysManager::doFlightModeAction()
{
    int flightState = RfkillSwitch::instance()->getCurrentFlightMode();

    if(flightState == -1) {
        USD_LOG(LOG_ERR,"get flight mode error");
        return;
    }

    mDeviceWindow->setAction(flightState?"ukui-airplane-on":"ukui-airplane-off");
    mDeviceWindow->dialogShow();
}

void MediaKeysManager::doEyeCenterAction()
{
    executeCommand("eye-protection-center","");
}

void MediaKeysManager::doUrlAction(const QString scheme)
{
    GError *error = NULL;
    GAppInfo *appInfo;

    appInfo = g_app_info_get_default_for_uri_scheme (scheme.toLatin1().data());

    if (appInfo != NULL) {
       if (!g_app_info_launch (appInfo, NULL, NULL, &error)) {
            qWarning("Could not launch '%s': %s",
                    g_app_info_get_commandline (appInfo),
                    error->message);
            g_object_unref (appInfo);
            g_error_free (error);
        }
    }else
        qWarning("Could not find default application for '%s' scheme",
                   scheme.toLatin1().data());
}

void MediaKeysManager::doWebcamAction()
{

    QDBusInterface *iface = new QDBusInterface("org.ukui.authority", \
                           "/", \
                           "org.ukui.authority.interface", \
                           QDBusConnection::systemBus());

    QDBusReply<QString> reply = iface->call("getCameraBusinfo");
    if (reply.isValid()){
        QString businfo = reply.value();
        QDBusReply<QString> reply2 = iface->call("toggleCameraDevice", businfo);

        if (reply2.isValid()){
            QString result = reply2.value();

            if (result == QString("binded")){
                mDeviceWindow->setAction("ukui-camera-on");
                iface->call("setCameraKeyboardLight", false);
            } else if (result == QString("unbinded")){
                mDeviceWindow->setAction("ukui-camera-off");

                iface->call("setCameraKeyboardLight", true);
            } else {
                USD_LOG(LOG_DEBUG,"toggleCameraDevice result %s", result.toLatin1().data());
            }
            mDeviceWindow->dialogShow();

        } else {
            USD_LOG(LOG_ERR,"Toggle Camera device Failed!");
        }

    } else {
        USD_LOG(LOG_ERR,"Get Camera Businfo Failed!");
    }
    delete iface;
}

/**
 * @brief MediaKeysManager::doMultiMediaPlayerAction
 *        for detailed purposes,refer to the definition of MediaPlayerKeyPressed()
 *        有关该函数的详细用途，请参考MediaPlayerKeyPressed()的声明
 * @param operation
 *        @operation can take the following values: Play、Pause、Stop...
 */
void MediaKeysManager::doMultiMediaPlayerAction(const QString operation)
{
    if(!mediaPlayers.isEmpty())
        Q_EMIT MediaPlayerKeyPressed(mediaPlayers.first()->application,operation);
}


/**
 * @brief MediaKeysManager::GrabMediaPlayerKeys
 *        this is a dbus method,it will be called follow org.ukui.SettingsDaemon.MediaKeys in mpris plugin
 *        这是一个dbus method,它将会跟随org.ukui.SettingsDaemon.MediaKeys这个dbus在mpris插件中调用
 * @param app
 *        app=="UsdMpris" is true according to the open source.
 *        按照开源的写法，这个变量的值为"UsdMpris"
 * @param time
 */
void MediaKeysManager::GrabMediaPlayerKeys(QString app)
{
    QTime currentTime;
    uint curTime = 0;       //current time(s) 当前时间(秒)
    bool containApp;


    currentTime = QTime::currentTime();
    curTime = 60*currentTime.minute() + currentTime.second() + currentTime.msec()/1000;

    //whether @app is inclued in @mediaPlayers  @mediaPlayers中是否包含@app
    containApp = findMediaPlayerByApplication(app);

    if(true == containApp)
        removeMediaPlayerByApplication(app,curTime);

    MediaPlayer* newPlayer = new MediaPlayer;
    newPlayer->application = app;
    newPlayer->time = curTime;
    mediaPlayers.insert(findMediaPlayerByTime(newPlayer),newPlayer);

}

void MediaKeysManager::mediaKeyForOtherApp(int action,QString appName)
{
    Q_UNUSED(action);
    Q_UNUSED(appName);
//    USD_LOG(LOG_DEBUG,"action:%d appName:%s",action, appName.toLatin1().data());
//    doAction(action);
}
/**
 * @brief MediaKeysManager::ReleaseMediaPlayerKeys
 * @param app
 */
void MediaKeysManager::ReleaseMediaPlayerKeys(QString app)
{
    bool containApp;
    QList<MediaPlayer*>::iterator item,end;
    MediaPlayer* tmp;

    item = mediaPlayers.begin();
    end = mediaPlayers.end();

    containApp = findMediaPlayerByApplication(app);

    if(true == containApp){
        for(; item != end; ++item){
            tmp = *item;
            //find @player successfully 在链表中成功找到该元素
            if(tmp->application == app){
                tmp->application.clear();
                delete tmp;
                mediaPlayers.removeOne(tmp);
                break;
            }
        }
    }
}

/**
 * @brief MediaKeysManager::findMediaPlayerByApplication
 * @param app   app=="UsdMpris" is true at general 一般情况下app=="UsdMpris"
 * @param player    if@app can be founded in @mediaPlayers, @player recored it's position
 *                  如果@app 存在于@mediaPlayers内，则@player 记录它的位置
 * @return return true if @app can be founded in @mediaPlayers
 */
bool MediaKeysManager::findMediaPlayerByApplication(const QString& app)
{
    QList<MediaPlayer*>::iterator item,end;
    MediaPlayer* tmp;

    item = mediaPlayers.begin();
    end = mediaPlayers.end();

    for(; item != end; ++item){
        tmp = *item;
        //find @player successfully 在链表中成功找到该元素
        if(tmp->application == app){
            return true;
        }
    }

    //can not find @player at QList<MediaPlayer*> 查找失败
    return false;
}

/**
 * @brief MediaKeysManager::findMediaPlayerByTime determine insert location at the @mediaPlayers
 *        确定在@mediaPlayers中的插入位置
 * @param player
 * @return  return index.    返回下标
 */
uint MediaKeysManager::findMediaPlayerByTime(MediaPlayer* player)
{
    if(mediaPlayers.isEmpty())
        return 0;
    return player->time < mediaPlayers.first()->time;
}

void MediaKeysManager::removeMediaPlayerByApplication(const QString& app,uint currentTime)
{
    QList<MediaPlayer*>::iterator item,end;
    MediaPlayer* tmp;

    item = mediaPlayers.begin();
    end = mediaPlayers.end();

    for(; item != end; ++item){
        tmp = *item;
        //find @player successfully 在链表中成功找到该元素
        if(tmp->application == app && tmp->time < currentTime){
            tmp->application.clear();
            delete tmp;
            mediaPlayers.removeOne(tmp);
            break;
        }
    }
}

int  MediaKeysManager::getFlightState()
{
    return RfkillSwitch::instance()->getCurrentFlightMode();
}
void MediaKeysManager::setFlightState(int value)
{
    RfkillSwitch::instance()->toggleFlightMode(value);
}
