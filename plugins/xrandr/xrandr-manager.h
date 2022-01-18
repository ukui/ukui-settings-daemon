

/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>
 * Copyright 2016 by Sebastian Kügler <sebas@kde.org>
 * Copyright (c) 2018 Kai Uwe Broulik <kde@broulik.de>
 *                    Work sponsored by the LiMux project of
 *                    the city of Munich.
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
#ifndef XRANDRMANAGER_H
#define XRANDRMANAGER_H

#include <QObject>
#include <QTimer>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QGSettings/qgsettings.h>
#include <QScreen>


#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/log.h>
#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/edid.h>
#include <KF5/KScreen/kscreen/configmonitor.h>
#include <KF5/KScreen/kscreen/getconfigoperation.h>
#include <KF5/KScreen/kscreen/setconfigoperation.h>
#include <usd_base_class.h>
#include "xrandr-dbus.h"
#include "xrandr-adaptor.h"
#include "xrandr-config.h"
#include "usd_base_class.h"
#include "usd_global_define.h"

#define SAVE_CONFIG_TIME 800

typedef struct _MapInfoFromFile
{
    QString sTouchName;     //触摸屏的名称
    QString sTouchSerial;   //触摸屏的序列号
    QString sMonitorName;   //显示器的名称
}MapInfoFromFile;             //配置文件中记录的映射关系信息
//END 触摸屏自动映射相关

typedef struct _TouchpadMap
{
    int     sTouchId;
    QString sMonitorName;   //显示器的名称
}touchpadMap;             //配置文件中记录的映射关系信息
//END 触摸屏自动映射相关

class XrandrManager: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KScreen")

public:
    XrandrManager();
    ~XrandrManager() override;

public:
    bool XrandrManagerStart();
    void XrandrManagerStop();
    void StartXrandrIdleCb();
    void monitorsInit();
    void changeScreenPosition();
    void applyConfig();
    void applyKnownConfig(bool state);
    void applyIdealConfig();
    void outputConnectedChanged();
    void doApplyConfig(const KScreen::ConfigPtr &config);
    void doApplyConfig(std::unique_ptr<xrandrConfig> config);
    void refreshConfig();


    UsdBaseClass::eScreenMode discernScreenMode();

    void saveCurrentConfig();
    void setMonitorForChanges(bool enabled);
    static void printScreenApplyFailReason(KScreen::ConfigPtr KsData);


    void orientationChangedProcess(Qt::ScreenOrientation orientation);
    void init_primary_screens(KScreen::ConfigPtr config);

    void primaryScreenChange();
    void callMethod(QRect geometry, QString name);
    void calingPeonyProcess();
    bool pretreatScreenInfo();
    void outputChangedHandle(KScreen::Output *senderOutput);
    void lightLastScreen();
    void outputConnectedWithoutConfigFile(KScreen::Output *senderOutput ,char outputCount);
    void setScreenModeToClone();
    void setScreenModeToFirst(bool isFirstMode);
    void setScreenModeToExtend();
    bool checkPrimaryScreenIsSetable();
    bool readAndApplyScreenModeFromConfig(UsdBaseClass::eScreenMode eMode);
    int8_t getCurrentMode();
    uint8_t getCurrentRotation();
    void sendScreenModeToDbus();

    void autoRemapTouchscreen();
    void remapFromConfig(QString mapPath);
    void SetTouchscreenCursorRotation();
    void doRemapAction (int input_name, char *output_name , bool isRemapFromFile = false);
    bool checkScreenByName(QString screenName);
    bool checkMapTouchDeviceById(int id);
    bool checkMapScreenByName(const QString screenName);
public Q_SLOTS:
    void TabletSettingsChanged(const bool tablemode);
    void configChanged();
    void RotationChangedEvent(const QString &rotation);
    void outputAddedHandle(const KScreen::OutputPtr &output);
    void outputRemoved(int outputId);
    void primaryOutputChanged(const KScreen::OutputPtr &output);
   // void applyConfigTimerHandle();
    void setScreenMode(QString modeName);
    void setScreensParam(QString screensParam);
    void SaveConfigTimerHandle();
    QString getScreesParam();
    void screenModeChangedSignal(int mode);
    void screensParamChangedSignal(QString param);
    /*台式机screen旋转后触摸*/
    void controlScreenMap(const QString screenMap);

Q_SIGNALS:
    // DBus
    void outputConnected(const QString &outputName);
    void unknownOutputConnected(const QString &outputName);

private:

    QList<touchpadMap*>    mTouchMapList; //存储已映射的关系
    Q_INVOKABLE void getInitialConfig();
    QDBusInterface        *t_DbusTableMode;
    QDBusInterface        *m_DbusRotation;
    QTimer                *mAcitveTime = nullptr;
    QTimer                *mKscreenInitTimer = nullptr;
    QTimer                *mSaveConfigTimer = nullptr;
    QTimer                *mChangeCompressor = nullptr;
    QTimer                *mApplyConfigTimer = nullptr;
    QGSettings            *mXrandrSetting = nullptr;
    QGSettings            *mXsettings = nullptr;

    double                 mScale = 1.0;
    QDBusInterface        *mLoginInter;
    QDBusInterface        *mUkccDbus;
    std::unique_ptr<xrandrConfig> mMonitoredConfig = nullptr;
    KScreen::ConfigPtr mConfig = nullptr;
    xrandrDbus *mDbus;

    QMetaEnum metaEnum;

    bool mMonitoring;
    bool mConfigDirty = true;
    bool mSleepState = false;
    bool mAddScreen = false;
    QScreen *mScreen = nullptr;
    bool mStartingUp = true;
    bool mIsApplyConfigWhenSave = false;
};

#endif // XRANDRMANAGER_H
