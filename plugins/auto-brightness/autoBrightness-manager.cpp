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
  , m_brightnessThread(NULL)
{
    m_Sensor = new QLightSensor(this);
    m_AutoBrightnessSettings  = new QGSettings(SETTINGS_AUTO_BRIGHTNESS_SCHEMAS);
    m_brightnessThread = new BrightThread();

    //进入空闲时，停止传感器采集，退出空闲时根据auto-brightness的键值决定是否开启传感器采集，并直接设置亮度。（auto-brightness为true时退出空闲模式代替电源管理调节亮度）
    QDBusConnection::sessionBus().connect(
        QString(),
        QString(SESSION_MANAGER_PATH),
        GNOME_SESSION_MANAGER,
        "StatusChanged",
        this,
        SLOT(idleModeChangeSlot(quint32)));

    m_Sensor->start();
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

    if (m_brightnessThread) {
        m_brightnessThread->stopImmediately();
        m_brightnessThread->deleteLater();
    }
}

AutoBrightnessManager *AutoBrightnessManager::autoBrightnessManagerNew()
{
    if (nullptr == m_AutoBrightnessManager) {
        m_AutoBrightnessManager = new AutoBrightnessManager();
    }

    return m_AutoBrightnessManager;
}

void AutoBrightnessManager::sensorReadingChangedSlot()
{
    QLightReading * lightReading = m_Sensor->reading();
    if (nullptr == lightReading) {
        USD_LOG(LOG_DEBUG,"read error....");
        return ;
    }

    qreal realTimeLux = lightReading->lux();
    adjustBrightnessWithLux(realTimeLux);
}

void AutoBrightnessManager::sensorActiveChangedSlot()
{
    sensorReadingChangedSlot();
}

void AutoBrightnessManager::setEnabled(bool enabled)
{
    if(m_Enabled == enabled) {
        return;
    }

    m_Enabled = enabled;

    if (m_Enabled) {
        m_Sensor->start();
        sensorActiveChangedSlot();
    } else {
        if (m_brightnessThread) {
            m_brightnessThread->stopImmediately();
        }
        m_Sensor->stop();
    }
}

void AutoBrightnessManager::gsettingsChangedSlot(QString key)
{
    int debugLux = 0;

    if (key == AUTO_BRIGHTNESS_KEY) {
        m_enableAutoBrightness = m_AutoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();
        enableSensorAndSetGsettings(m_enableAutoBrightness);
    } else if (key == DEBUG_LUX_KEY) {
        if (m_AutoBrightnessSettings->get(DEBUG_MODE_KEY).toBool()) {
            debugLux = m_AutoBrightnessSettings->get(DEBUG_LUX_KEY).toInt();
            adjustBrightnessWithLux(debugLux);
        }
    } else if (key == DEBUG_MODE_KEY) {//开启调试就要禁用自动
        m_enableAutoBrightness =!(m_AutoBrightnessSettings->get(DEBUG_MODE_KEY).toBool());
        enableSensorAndSetGsettings(m_enableAutoBrightness);
    }
}

void AutoBrightnessManager::idleModeChangeSlot(quint32 mode)
{

    USD_LOG_SHOW_PARAM1(mode);
    USD_LOG(LOG_DEBUG,"get session mode");

    if (m_enableAutoBrightness == false) {
        return;
    }

    if (mode == SESSION_BUSY) {
        setEnabled(false);//尽量操作硬件停止向上层发送数据，减少系统开销。
    } else if (mode == SESSION_IDLE) {
        setEnabled(true);
    }
}

void AutoBrightnessManager::enableSensorAndSetGsettings(bool state)
{
    setEnabled(state);
}

void AutoBrightnessManager::adjustBrightnessWithLux(qreal realTimeLux)
{
    int realTimeBrightness;
    USD_LOG(LOG_DEBUG,"realTime lux: %f", realTimeLux);

    realTimeBrightness = m_brightnessThread->getRealTimeBrightness();

    if (realTimeBrightness < 0) {
        USD_LOG(LOG_DEBUG,"get brightness error");
        return;
    }

    if(realTimeLux >= BRIGHTNESS_40_LOW_LIMIT && realTimeLux < BRIGHTNESS_40_UP_LIMIT) {
        //40%
        m_brightnessThread->setBrightness(40);
    } else if (realTimeLux >= BRIGHTNESS_40_UP_LIMIT && realTimeLux < BRIGHTNESS_80_LOW_LIMIT) {
        if (realTimeBrightness == 40 || realTimeBrightness == 80) {
            return;
        }
        m_brightnessThread->setBrightness(40);
    } else if (realTimeLux >= BRIGHTNESS_80_LOW_LIMIT && realTimeLux < BRIGHTNESS_80_UP_LIMIT) {
        //80%
        m_brightnessThread->setBrightness(80);
    } else if(realTimeLux >= BRIGHTNESS_80_UP_LIMIT && realTimeLux < BRIGHTNESS_100_LOW_LIMIT) {
        if (realTimeBrightness == 100 || realTimeBrightness == 80) {
            return;
        }
        m_brightnessThread->setBrightness(80);
    } else if(realTimeLux >= BRIGHTNESS_100_LOW_LIMIT) {
        //100%
        m_brightnessThread->setBrightness(100);
    }

    m_brightnessThread->start();
}

bool AutoBrightnessManager::autoBrightnessManagerStart()
{
    bool readSensorDataSuccess = true;

    USD_LOG(LOG_DEBUG, "AutoBrightnessManager Start");

    m_enableAutoBrightness = m_AutoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();
    m_hadSensor = m_AutoBrightnessSettings->get(HAD_SENSOR_KEY).toBool();

    QLightReading * lightReading = m_Sensor->reading();

    if (nullptr == lightReading) {
        readSensorDataSuccess = false;
    }

    if (m_hadSensor != readSensorDataSuccess) {//如果读不到传感器的值就认为没有传感器,读到则为有传感器，该值提供给护眼中心使用。
        m_AutoBrightnessSettings->set(HAD_SENSOR_KEY, readSensorDataSuccess);
    }

    m_Sensor->stop();//stop为了停止为上一次读取而设置的start

    if (false == readSensorDataSuccess) {
        if (m_enableAutoBrightness == true) {
            m_AutoBrightnessSettings->set(AUTO_BRIGHTNESS_KEY, false);
        }
        USD_LOG(LOG_DEBUG, "can't find lux sensor...");
        return true;
    }

    USD_LOG(LOG_DEBUG, "find lux sensor...");

    connect(m_Sensor, SIGNAL(readingChanged()), this, SLOT(sensorReadingChangedSlot()));
    connect(m_Sensor, SIGNAL(activeChanged()), this, SLOT(sensorActiveChangedSlot()));
    connect(m_AutoBrightnessSettings, SIGNAL(changed(QString)), this, SLOT(gsettingsChangedSlot(QString)));

    enableSensorAndSetGsettings(m_enableAutoBrightness);
    return true;
}

void AutoBrightnessManager::autoBrightnessManagerStop()
{
    setEnabled(false);
    USD_LOG(LOG_DEBUG, "AutoBrightness Manager stop");
}
