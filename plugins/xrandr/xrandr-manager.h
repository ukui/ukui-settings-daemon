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

#include "xrandr-dbus.h"
#include "xrandr-adaptor.h"
#include "xrandr-config.h"
#include "usd_base_class.h"

class XrandrManager: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KScreen")

public:
    XrandrManager();
    ~XrandrManager() override;


    //coolDownStart 1.5->>coolDowning3.5->coolDownStart
    //coolDownStart 1.5->>coolDowning3.5(有事件)->coolDownRun-》coolDownStart
    enum ScreenCanBeApplyStatus {
        coolDownStart,//可以开始执行1.5s计时，超时后应用配置
        coolDownOverAndRun,    //CD结束后立刻应用配置（配置文件已更新）
        coolDowning,        //冷却中（只更新配置，不应用配置）
    };
public:
    bool XrandrManagerStart();
    void XrandrManagerStop();
    void StartXrandrIdleCb ();
    void monitorsInit();
    void changeScreenPosition();
    void applyConfig();
    void applyKnownConfig(bool state);
    void applyIdealConfig();
    void outputConnectedChanged();
    void doApplyConfig(const KScreen::ConfigPtr &config);
    void doApplyConfig(std::unique_ptr<xrandrConfig> config);
    void refreshConfig();


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
public Q_SLOTS:
    void configChanged();
    void mPrepareForSleep(bool);
    void RotationChangedEvent(QString);
    void outputAddedHandle(const KScreen::OutputPtr &output);
    void outputRemoved(int outputId);
    void primaryOutputChanged(const KScreen::OutputPtr &output);
//    void applyConfigTimerHandle();

Q_SIGNALS:
    // DBus
    void outputConnected(const QString &outputName);
    void unknownOutputConnected(const QString &outputName);

private:

    Q_ENUM(ScreenCanBeApplyStatus)
    Q_INVOKABLE void getInitialConfig();
    QTimer                *mAcitveTime = nullptr;
    QTimer                *mSaveTimer = nullptr;
    QTimer                *mChangeCompressor = nullptr;
    QTimer                *mApplyConfigTimer = nullptr;
    QGSettings            *mXrandrSetting = nullptr;
    QGSettings            *mXsettings = nullptr;
    double                 mScale = 1.0;
    QDBusInterface        *mLoginInter;
    std::unique_ptr<xrandrConfig> mMonitoredConfig = nullptr;
    KScreen::ConfigPtr mConfig = nullptr;
    xrandrDbus *mDbus;
    bool mMonitoring;
    bool mConfigDirty = true;
    bool mSleepState = false;
    bool mAddScreen = false;
    QScreen *mScreen = nullptr;
    bool mStartingUp = true;
    ScreenCanBeApplyStatus mScreenCanBeApply = XrandrManager::coolDownStart;
};

#endif // XRANDRMANAGER_H
