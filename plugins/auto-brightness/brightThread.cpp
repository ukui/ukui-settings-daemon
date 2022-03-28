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


#include "QGSettings/qgsettings.h"

extern "C"{
#include <math.h>
#include <unistd.h>
#include "clib-syslog.h"
}

#define SETTINGS_POWER_MANAGER  "org.ukui.power-manager"
#define AUTOBRIGHTNESS_GSETTING_SCHEMA  "org.ukui.SettingsDaemon.plugins.auto-brightness"
#define BRIGHTNESS_AC           "brightness-ac"
#define DELAYMS                 "delayms"
//空闲模式不进行亮度处理

BrightThread::BrightThread(QObject *parent)
{
    bool ret = false;
    m_powerSettings = new QGSettings(SETTINGS_POWER_MANAGER);

    if (nullptr == m_powerSettings) {
        USD_LOG(LOG_DEBUG,"can't find %s", SETTINGS_POWER_MANAGER);
    }

    m_brightnessSettings = new QGSettings(AUTOBRIGHTNESS_GSETTING_SCHEMA);
    if (nullptr == m_brightnessSettings) {
        return;
    }

    m_delayms = m_brightnessSettings->get(DELAYMS).toInt(&ret);
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

    if (false == m_powerSettings->keys().contains(BRIGHTNESS_AC)) {
        return;
    }

    currentBrightnessValue = m_powerSettings->get(BRIGHTNESS_AC).toInt();
    USD_LOG(LOG_DEBUG,"start set brightness");

    while(currentBrightnessValue != m_destBrightness) {
        if (currentBrightnessValue>m_destBrightness){
            currentBrightnessValue--;
        } else {
            currentBrightnessValue++;
        }

        m_powerSettings->set(BRIGHTNESS_AC,currentBrightnessValue);
        m_powerSettings->apply();
        msleep(m_delayms);//30 ms效果较好。
    }
    USD_LOG(LOG_DEBUG,"set brightness over");
}

void BrightThread::stopImmediately(){

}


void BrightThread::setBrightness(int brightness)
{
    m_destBrightness = brightness;
}

int BrightThread::getRealTimeBrightness()
{
    if (false == m_powerSettings->keys().contains(BRIGHTNESS_AC)) {
       return -1;
    }

    return m_powerSettings->get(BRIGHTNESS_AC).toInt();
}

BrightThread::~BrightThread()
{
    if(m_powerSettings)
        delete m_powerSettings;
}
