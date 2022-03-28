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

#include "brightThread.h"
#include <QByteArray>
#include <QDebug>
#include <QMutexLocker>
#include <QDBusInterface>
#include <QApplication>

#include "usd_global_define.h"
#include "QGSettings/qgsettings.h"

extern "C"{
#include <math.h>
#include <unistd.h>
#include "clib-syslog.h"
}

//空闲模式不进行亮度处理

BrightThread::BrightThread(QObject *parent):
    m_stop(false)
{
    bool ret = false;
    m_powerSettings = new QGSettings(POWER_MANAGER_SCHEMA);

    if (nullptr == m_powerSettings) {
        USD_LOG(LOG_DEBUG,"can't find %s", POWER_MANAGER_SCHEMA);
    }

    m_brightnessSettings = new QGSettings(AUTO_BRIGHTNESS_SCHEMA);
    if (nullptr == m_brightnessSettings) {
        return;
    }

    m_delayms = m_brightnessSettings->get(DELAYMS_KEY).toInt(&ret);

    if (false == ret) {
        USD_LOG(LOG_DEBUG,"can't find delayms");
        m_delayms = 30;
    }

    USD_LOG_SHOW_PARAM1(m_delayms);
}


void BrightThread::run(){
    int currentBrightnessValue;

    if (nullptr == m_powerSettings) {
        return;
    }

    if (false == m_powerSettings->keys().contains(BRIGHTNESS_AC_KEY)) {
        return;
    }

    currentBrightnessValue = m_powerSettings->get(BRIGHTNESS_AC_KEY).toInt();
    USD_LOG(LOG_DEBUG,"start set brightness");

    m_stop = false;
    while(currentBrightnessValue != m_destBrightness) {

        if (m_stop) {
            return;
        }

        if (currentBrightnessValue>m_destBrightness){
            currentBrightnessValue--;
        } else {
            currentBrightnessValue++;
        }

        m_powerSettings->set(BRIGHTNESS_AC_KEY,currentBrightnessValue);
        m_powerSettings->apply();
        msleep(m_delayms);//30 ms效果较好。
    }
    USD_LOG(LOG_DEBUG,"set brightness over");
}

void BrightThread::stopImmediately(){
    m_stop = true;
}


void BrightThread::setBrightness(int brightness)
{
    m_destBrightness = brightness;
}

int BrightThread::getRealTimeBrightness()
{
    if (false == m_powerSettings->keys().contains(BRIGHTNESS_AC_KEY)) {
       return -1;
    }

    return m_powerSettings->get(BRIGHTNESS_AC_KEY).toInt();
}

BrightThread::~BrightThread()
{
    if (m_powerSettings) {
        delete m_powerSettings;
    }

    if (m_brightnessSettings) {
        delete m_brightnessSettings;
    }
}
