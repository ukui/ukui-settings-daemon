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


AutoBrightnessManager *AutoBrightnessManager::m_autoBrightnessManager = nullptr;
//如果开启自动背光，系统由空闲进入忙时，电源管理不进行亮度处理，由usd的光感模式处理
AutoBrightnessManager::AutoBrightnessManager():
    m_enabled(false)
  , m_brightnessThread(NULL)
  ,m_userIntervene(false)
{
    m_lightSensor = new QLightSensor(this);
    m_autoBrightnessSettings  = new QGSettings(AUTO_BRIGHTNESS_SCHEMA);
    m_powerManagerSettings = new QGSettings(POWER_MANAGER_SCHEMA);
    m_lightSensor->start();//预先读取一次，为了确保sensor存在，取到后直接关闭。
}

AutoBrightnessManager::~AutoBrightnessManager()
{
    if (m_autoBrightnessManager) {
        delete m_autoBrightnessManager;
    }

    if (m_lightSensor) {
        delete m_lightSensor;
    }

    if (m_autoBrightnessSettings) {
        delete m_autoBrightnessSettings;
    }

    if (m_brightnessThread) {
        m_brightnessThread->stopImmediately();
        m_brightnessThread->deleteLater();
    }

    if (m_powerManagerSettings) {
        m_powerManagerSettings->deleteLater();
    }
}

AutoBrightnessManager *AutoBrightnessManager::autoBrightnessManagerNew()
{
    if (nullptr == m_autoBrightnessManager) {
        m_autoBrightnessManager = new AutoBrightnessManager();
    }

    return m_autoBrightnessManager;
}

void AutoBrightnessManager::sensorReadingChangedSlot()
{
    QLightReading * lightReading = m_lightSensor->reading();
    if (nullptr == lightReading || false == m_lightSensor->isActive()) {
        USD_LOG(LOG_DEBUG, "lux read error....");
        return;
    }

    qreal realTimeLux = lightReading->lux();
    adjustBrightnessWithLux(realTimeLux);
}

void AutoBrightnessManager::sensorActiveChangedSlot()
{
    USD_LOG_SHOW_PARAM1(m_lightSensor->isActive());
    sensorReadingChangedSlot();
}

void AutoBrightnessManager::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }

    m_enabled = enabled;

    if (m_enabled) {
        m_lightSensor->setActive(true);
        m_lightSensor->start();
        sensorActiveChangedSlot();
    } else {
        if (m_brightnessThread) {
            m_brightnessThread->stopImmediately();
        }

        m_lightSensor->setActive(false);
        m_lightSensor->stop();
    }
}

void AutoBrightnessManager::gsettingsChangedSlot(QString key)
{
    int debugLux = 0;

    if (key == AUTO_BRIGHTNESS_KEY) {
        m_enableAutoBrightness = m_autoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();
        enableSensorAndSetGsettings(m_enableAutoBrightness);
    } else if (key == DEBUG_LUX_KEY) {
        if (m_autoBrightnessSettings->get(DEBUG_MODE_KEY).toBool()) {
            if (m_userIntervene) {
                return;
            }

            debugLux = m_autoBrightnessSettings->get(DEBUG_LUX_KEY).toInt();
            adjustBrightnessWithLux(debugLux);
        }
    } else if (key == DEBUG_MODE_KEY) {//开启调试就要禁用自动
        m_enableAutoBrightness =!(m_autoBrightnessSettings->get(DEBUG_MODE_KEY).toBool());
        enableSensorAndSetGsettings(m_enableAutoBrightness);
    }
}

void AutoBrightnessManager::idleModeChangeSlot(quint32 mode)
{

    if (m_enableAutoBrightness == false) {
        USD_LOG_SHOW_PARAM1(m_enableAutoBrightness);
        return;
    }

    if (mode == SESSION_BUSY) {
        setEnabled(true);//尽量操作硬件停止向上层发送数据，减少系统开销。
    } else if (mode == SESSION_IDLE) {
        m_userIntervene = false; //用户干预后空闲模式才进行恢复自动控制
        setEnabled(false);
    }
}

void AutoBrightnessManager::powerManagerSchemaChangedSlot(QString key)
{
    if (key == BRIGHTNESS_AC_KEY) {
        m_userIntervene = true;
        setEnabled(false);
    }
}

void AutoBrightnessManager::brightnessThreadFinishedSlot()
{
    USD_LOG(LOG_DEBUG,"brightness had finished...");
    connectPowerManagerSchema(true);
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

    connectPowerManagerSchema(false);
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

void AutoBrightnessManager::connectPowerManagerSchema(bool state)
{
    if (state) {
         connect(m_powerManagerSettings, SIGNAL(changed(QString)), this, SLOT(powerManagerSchemaChangedSlot(QString)));
    } else {
         disconnect(m_powerManagerSettings, SIGNAL(changed(QString)), this, SLOT(powerManagerSchemaChangedSlot(QString)));
    }
}

bool AutoBrightnessManager::autoBrightnessManagerStart()
{
    bool readSensorDataSuccess = true;

    USD_LOG(LOG_DEBUG, "AutoBrightnessManager Start");

    m_enableAutoBrightness = m_autoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();
    m_hadSensor = m_autoBrightnessSettings->get(HAD_SENSOR_KEY).toBool();

    QLightReading * lightReading = m_lightSensor->reading();

    if (nullptr == lightReading) {
        readSensorDataSuccess = false;
    }

    if (m_hadSensor != readSensorDataSuccess) {//如果读不到传感器的值就认为没有传感器,读到则为有传感器，该值提供给护眼中心使用。
        m_autoBrightnessSettings->set(HAD_SENSOR_KEY, readSensorDataSuccess);
    }

    m_lightSensor->stop();//读取数据后即可停止，为了节省开销。

    if (false == readSensorDataSuccess) {
        if (m_enableAutoBrightness == true) {
            m_autoBrightnessSettings->set(AUTO_BRIGHTNESS_KEY, false);
        }

        USD_LOG(LOG_DEBUG, "can't find lux sensor...");
        return true;
    }

    USD_LOG(LOG_DEBUG, "find lux sensor AutoBrightness:%d", m_enableAutoBrightness);
    //进入空闲时，停止传感器采集，退出空闲时根据auto-brightness的键值决定是否开启传感器采集，并直接设置亮度。（auto-brightness为true时退出空闲模式代替电源管理调节亮度）
    QDBusConnection::sessionBus().connect(
        QString(),
        QString(SESSION_MANAGER_PATH),
        GNOME_SESSION_MANAGER,
        "StatusChanged",
        this,
        SLOT(idleModeChangeSlot(quint32)));

    m_lightSensor->setActive(true);
    m_lightSensor->start();//
    m_brightnessThread = new BrightThread();

    connect(m_lightSensor, SIGNAL(readingChanged()), this, SLOT(sensorReadingChangedSlot()));
    connect(m_lightSensor, SIGNAL(activeChanged()), this, SLOT(sensorActiveChangedSlot()));
    connect(m_autoBrightnessSettings, SIGNAL(changed(QString)), this, SLOT(gsettingsChangedSlot(QString)));
    connect(m_brightnessThread, SIGNAL(finished()), this, SLOT(brightnessThreadFinishedSlot()));
    connectPowerManagerSchema(true);

    enableSensorAndSetGsettings(m_enableAutoBrightness);
    return true;
}

void AutoBrightnessManager::autoBrightnessManagerStop()
{
    setEnabled(false);
    USD_LOG(LOG_DEBUG, "AutoBrightness Manager stop");
}
