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
#include "usd_global_define.h"
#include "autoBrightness-manager.h"

extern "C"{
#include <clib-syslog.h>
#include <unistd.h>
#include <math.h>
}

#define BRIGHTNESS_40_LOW_LIMIT 0
#define BRIGHTNESS_40_UP_LIMIT 70

#define BRIGHTNESS_80_LOW_LIMIT 90
#define BRIGHTNESS_80_UP_LIMIT 600

#define BRIGHTNESS_100_LOW_LIMIT 800
#define BRIGHTNESS_100_UP_LIMIT 9999999


AutoBrightnessManager *AutoBrightnessManager::m_AutoBrightnessManager = nullptr;
//如果开启自动背光，系统由空闲进入忙时，电源管理不进行亮度处理，由usd的光感模式处理
AutoBrightnessManager::AutoBrightnessManager() :
    m_Enabled(false)
  , m_currentRunLocalThread(NULL)
{
    m_Sensor = new QLightSensor(this);
    m_AutoBrightnessSettings  = new QGSettings(SETTINGS_AUTO_BRIGHTNESS_SCHEMAS);
    m_currentRunLocalThread = new BrightThread();

    //进入空闲时，停止传感器采集，退出空闲时根据auto-brightness的键值决定是否开启传感器采集，并直接设置亮度。（auto-brightness为true时退出空闲模式代替电源管理调节亮度）
    QDBusConnection::sessionBus().connect(
        QString(),
        QString(SESSION_MANAGER_PATH),
        GNOME_SESSION_MANAGER,
        "StatusChanged",
        this,
        SLOT(idleModeChangeSlot(quint32)));
}

AutoBrightnessManager::~AutoBrightnessManager()
{
    if (m_AutoBrightnessManager) {
        delete m_AutoBrightnessManager;
    }

    if (m_Sensor) {
        delete m_Sensor;
    }

    if (m_AutoBrightnessSettings) {
        delete m_AutoBrightnessSettings;
    }

    if (m_currentRunLocalThread) {
        m_currentRunLocalThread->stopImmediately();
        m_currentRunLocalThread->deleteLater();
    }
}

AutoBrightnessManager *AutoBrightnessManager::autoBrightnessManagerNew()
{
    if (nullptr == m_AutoBrightnessManager) {
        m_AutoBrightnessManager = new AutoBrightnessManager();
    }

    return m_AutoBrightnessManager;
}

void AutoBrightnessManager::autoBrightnessUpdateState()
{
    int realTimeBrightness = 0;
    QLightReading * lightReading = m_Sensor->reading();
    if (nullptr == lightReading) {
        USD_LOG(LOG_DEBUG,"read error....");
        return ;
    }

    qreal realTimeLux = lightReading->lux();
    USD_LOG(LOG_DEBUG,"realTime lux: %f",realTimeLux);

    realTimeBrightness = m_currentRunLocalThread->getRealTimeBrightness();

    if (realTimeBrightness < 0) {
        USD_LOG(LOG_DEBUG,"get brightness error");
        return;
    }

    if(realTimeLux >= BRIGHTNESS_40_LOW_LIMIT && realTimeLux < BRIGHTNESS_40_UP_LIMIT) {
        //40%
        m_currentRunLocalThread->setBrightness(40);
    } else if (realTimeLux >= BRIGHTNESS_40_UP_LIMIT && realTimeLux < BRIGHTNESS_80_LOW_LIMIT) {
        if (realTimeBrightness == 40 || realTimeBrightness == 80) {
            return;
        }
        m_currentRunLocalThread->setBrightness(40);
    } else if (realTimeLux >= BRIGHTNESS_80_LOW_LIMIT && realTimeLux < BRIGHTNESS_80_UP_LIMIT) {
        //80%
        m_currentRunLocalThread->setBrightness(80);
    } else if(realTimeLux >= BRIGHTNESS_80_UP_LIMIT && realTimeLux < BRIGHTNESS_100_LOW_LIMIT) {
        if (realTimeBrightness == 100 || realTimeBrightness == 80) {
            return;
        }
        m_currentRunLocalThread->setBrightness(80);
    } else if(realTimeLux >= BRIGHTNESS_100_LOW_LIMIT) {
        //100%
        m_currentRunLocalThread->setBrightness(100);
    }
    m_currentRunLocalThread->start();
}

void AutoBrightnessManager::autoBrightnessRefresh()
{
    autoBrightnessUpdateState();
}

void AutoBrightnessManager::setEnabled(bool enabled)
{

    if(m_Enabled == enabled) {
        return;
    }

    m_Enabled = enabled;

    if (m_Enabled) {
        m_Sensor->start();
        autoBrightnessRefresh();
    } else {
        if(m_currentRunLocalThread) {
            m_currentRunLocalThread->stopImmediately();
        }
        m_Sensor->stop();
    }
}

void AutoBrightnessManager::autoBrightnessSettingsChanged(QString key)
{
    bool autobright;
    autobright = m_AutoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();

    if (key == AUTO_BRIGHTNESS_KEY) {
          setEnabled(autobright);
    }
}

void AutoBrightnessManager::idleModeChangeSlot(int mode)
{
    USD_LOG_SHOW_PARAM1(mode);
}

void AutoBrightnessManager::setAutoGsetings(bool state)
{
    bool sensorState = false;
    bool autoState = false;

    autoState = m_AutoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();
    sensorState = m_AutoBrightnessSettings->get(HAD_SENSOR_KEY).toBool();

    if (state == false && autoState == true) {//该值为人工设置。但是没有传感器不应为true，该值需要提供给电源管理使用
         m_AutoBrightnessSettings->set(AUTO_BRIGHTNESS_KEY,state);
    }

    if (state != sensorState) {
        m_AutoBrightnessSettings->set(HAD_SENSOR_KEY, state);
    }

    if (state==true && autoState == true) {
        setEnabled(true);
    } else {
        setEnabled(false);
    }
}

bool AutoBrightnessManager::autoBrightnessManagerStart()
{
    bool isActive = true;
    bool tempBool = false;
    bool autoState = false;

    USD_LOG(LOG_DEBUG, "AutoBrightnessManager Start");
    autoState = m_AutoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();
    tempBool = m_AutoBrightnessSettings->get(HAD_SENSOR_KEY).toBool();

    m_Sensor->start();
    QLightReading * lightReading = m_Sensor->reading();

    if (nullptr == lightReading) {
        isActive = false;
    }

    setAutoGsetings(isActive);
    //先读状态不对时再写入，减少对硬盘IO的操作。待检查？？

    if (false == isActive) {
        USD_LOG(LOG_DEBUG, "can't find lux sensor...");
        return true;
    }
    USD_LOG(LOG_DEBUG, "find lux sensor...");
    connect(m_Sensor, SIGNAL(readingChanged()), this, SLOT(autoBrightnessUpdateState()));
    connect(m_Sensor, SIGNAL(activeChanged()), this, SLOT(autoBrightnessRefresh()));
    connect(m_AutoBrightnessSettings, SIGNAL(changed(QString)), this, SLOT(autoBrightnessSettingsChanged(QString)));
    m_Sensor->start();
    autoBrightnessSettingsChanged(AUTO_BRIGHTNESS_KEY);
    return true;
}

void AutoBrightnessManager::autoBrightnessManagerStop()
{
    setEnabled(false);
    USD_LOG(LOG_DEBUG, "AutoBrightness Manager stop");
}
