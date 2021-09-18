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
#include <QApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusMessage>

#include <KConfigGroup>
#include "tabletMode-manager.h"

#define SETTINGS_XRANDR_SCHEMAS "org.ukui.SettingsDaemon.plugins.xrandr"
#define XRANDR_ROTATION_KEY     "xrandr-rotations"

#define SETTINGS_TABLET_SCHEMAS "org.ukui.SettingsDaemon.plugins.tablet-mode"
#define TABLET_AUTO_KEY         "auto-rotation"
#define TABLET_MODE_KEY         "tablet-mode"

TabletModeManager *TabletModeManager::mTabletManager = nullptr;

TabletModeManager::TabletModeManager()
{
    mSensor = new QOrientationSensor(this);
    mXrandrSettings = new QGSettings(SETTINGS_XRANDR_SCHEMAS);
    mTableSettings  = new QGSettings(SETTINGS_TABLET_SCHEMAS);


    t_DbusTableMode = new QDBusInterface("com.kylin.statusmanager.interface","/","com.kylin.statusmanager.interface",QDBusConnection::sessionBus(),this);

    if (t_DbusTableMode->isValid()) {
        connect(t_DbusTableMode, SIGNAL(mode_change_signal(bool)),this,SLOT(TabletSettingsChanged(bool)));
        //USD_LOG(LOG_DEBUG, "..");
    } else {
       //USD_LOG(LOG_DEBUG, "...");
    }

}

TabletModeManager::~TabletModeManager()
{
    if(mTabletManager)
        delete mTabletManager;
    if(mSensor)
        delete mSensor;
    if(mXrandrSettings)
        delete mXrandrSettings;
    if(mTableSettings)
        delete mTableSettings;
}

TabletModeManager *TabletModeManager::TabletModeManagerNew()
{
    if(nullptr == mTabletManager)
        mTabletManager = new TabletModeManager();
    return mTabletManager;
}

void TabletModeManager::TabletUpdateState()
{
    mSensor->reading();
    QOrientationReading *reading = mSensor->reading();
    switch (reading->orientation()) {
    case QOrientationReading::Undefined:
        qDebug("return TabletModeManager::Orientation::Undefined");
        //return TabletModeManager::Orientation::Undefined;
        break;
    case QOrientationReading::TopUp:
        qDebug("return TabletModeManager::Orientation::TopUp");
        mXrandrSettings->setEnum(XRANDR_ROTATION_KEY,0);
        break;
    case QOrientationReading::TopDown:
        qDebug("return TabletModeManager::Orientation::TopDown");
        mXrandrSettings->setEnum(XRANDR_ROTATION_KEY,2);
        break;
    case QOrientationReading::LeftUp:
        qDebug("return TabletModeManager::Orientation::LeftUp");
        mXrandrSettings->setEnum(XRANDR_ROTATION_KEY,1);
        break;
    case QOrientationReading::RightUp:
        qDebug("return TabletModeManager::Orientation::RightUp");
        mXrandrSettings->setEnum(XRANDR_ROTATION_KEY,3);
        break;
    case QOrientationReading::FaceUp:
        qDebug("return TabletModeManager::Orientation::FaceUp");
        break;
    case QOrientationReading::FaceDown:
        qDebug("return TabletModeManager::Orientation::FaceDown");
        break;
    default:
        Q_UNREACHABLE();
    }
}

void TabletModeManager::TabletRefresh()
{
    qDebug()<<__func__;
    if(mSensor->isActive()){
        qDebug()<<mSensor->isActive();
        TabletUpdateState();
    } else {
        qDebug()<<mSensor->isActive();
    }

}

void TabletModeManager::SetEnabled(bool enabled)
{
    if(mEnabled == enabled)
        return;
    mEnabled = enabled;
    if (mEnabled) {
        TabletRefresh();
        QDBusConnection::sessionBus().registerObject(QStringLiteral("/Orientation"), this);
    } else {
        QDBusConnection::sessionBus().unregisterObject(QStringLiteral("/Orientation"));
    }

    if (mEnabled) {
        mSensor->start();
    } else {
        mSensor->stop();
    }
}

void TabletModeManager::TabletSettingsChanged(const bool tablemode)
{
//    bool rotations, table;
//    rotations = mTableSettings->get(TABLET_AUTO_KEY).toBool();
//    table     = mTableSettings->get(TABLET_MODE_KEY).toBool();
//    if(table)
//        SetEnabled(rotations);

        if(tablemode){
            QDBusMessage message =
                    QDBusMessage::createSignal("/KGlobalSettings",
                                               "org.kde.KGlobalSettings",
                                               "send_to_client");
                message << bool(1);
                QDBusConnection::sessionBus().send(message);
        }else{
            QDBusMessage message =
                    QDBusMessage::createSignal("/KGlobalSettings",
                                               "org.kde.KGlobalSettings",
                                               "send_to_client");
                message << bool(0);
                QDBusConnection::sessionBus().send(message);
        }
      mTableSettings->set(TABLET_MODE_KEY, tablemode);
}

bool TabletModeManager::TabletModeManagerStart()
{
    qDebug("TabletMode Manager Start");
    bool rotations, table;
    rotations = mTableSettings->get(TABLET_AUTO_KEY).toBool();
    table     = mTableSettings->get(TABLET_MODE_KEY).toBool();
    connect(mSensor,SIGNAL(readingChanged()),this,SLOT(TabletUpdateState()));
    connect(mSensor,SIGNAL(activeChanged()),this,SLOT(TabletRefresh()));
    connect(mTableSettings,SIGNAL(changed(QString)),this,SLOT(TabletSettingsChanged(QString)));
    if(table)
        SetEnabled(rotations);
    return true;
}

void TabletModeManager::TabletModeManagerStop()
{
    SetEnabled(false);
    qDebug("TabletMode Manager stop");
}
