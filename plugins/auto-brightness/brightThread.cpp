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
#include <QGSettings/QGSettings>
#include <QByteArray>
#include <QDebug>
#include <QMutexLocker>

extern "C"{
#include <math.h>
#include <unistd.h>
}

#define SETTINGS_POWER_MANAGER  "org.ukui.power-manager"
#define BRIGHTNESS_AC           "brightness-ac"

BrightThread::BrightThread(QObject *parent, double bright) : QThread(parent), brightness(bright)
//BrightThread::BrightThread(QObject *parent) : QThread(parent)
{
    QByteArray id(SETTINGS_POWER_MANAGER);
    if(QGSettings::isSchemaInstalled(id))
        mpowerSettings = new QGSettings(SETTINGS_POWER_MANAGER);
    currentBrightness = mpowerSettings->get(BRIGHTNESS_AC).toDouble();
}

void BrightThread::run(){
    m_isCanRun = true;

    if(qAbs(brightness - currentBrightness) <= 0.01){
        mpowerSettings->set(BRIGHTNESS_AC, brightness);
        return;
    }
    if(brightness > currentBrightness){
        int interval = round(brightness-currentBrightness);
        double step = (brightness-currentBrightness)/interval;
        qDebug("需要调节%d次", interval);
        for(int i=1; i<=interval; i++){
//            qDebug() << "子线程ID：" << QThread::currentThreadId() << "m_isCanEun = " << m_isCanRun;
            {
                QMutexLocker locker(&m_lock);
                if(!m_isCanRun){
//                    qDebug() << QThread::currentThreadId() << "被终止";
                    return;
                }
            }
            mpowerSettings->set(BRIGHTNESS_AC, currentBrightness + i * step);
            usleep(50000);
        }
        mpowerSettings->set(BRIGHTNESS_AC, brightness);
    }
    else{
        int interval = round(currentBrightness-brightness);
        double step = (currentBrightness-brightness)/interval;
        qDebug("需要调节%d次", interval);
        for(int i=1; i<=interval; i++){
//            qDebug() << "子线程ID：" << QThread::currentThreadId() << "m_isCanEun = " << m_isCanRun;
            {
                QMutexLocker locker(&m_lock);
                if(!m_isCanRun){
//                    qDebug() << QThread::currentThreadId() << "被终止";
                    return;
                }
            }
            mpowerSettings->set(BRIGHTNESS_AC, currentBrightness - i * step);
            usleep(50000);
        }
        mpowerSettings->set(BRIGHTNESS_AC, brightness);
    }
    qDebug() << "brightness = " << brightness;
}

void BrightThread::stopImmediately(){
    QMutexLocker locker(&m_lock);
    m_isCanRun = false;
}

BrightThread::~BrightThread()
{
    if(mpowerSettings)
        delete mpowerSettings;
}
