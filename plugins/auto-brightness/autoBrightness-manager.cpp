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
#include "autoBrightness-manager.h"

extern "C"{
#include <unistd.h>
#include <math.h>
}

#define SETTINGS_AUTO_BRIGHTNESS_SCHEMAS "org.ukui.SettingsDaemon.plugins.auto-brightness"
#define AUTO_BRIGHTNESS_KEY              "auto-brightness"

AutoBrightnessManager *AutoBrightnessManager::mAutoBrightnessManager = nullptr;

AutoBrightnessManager::AutoBrightnessManager() :
    mEnabled(false)
  , m_currentRunLocalThread(NULL)
  , mPreLux(10.f)
{
    mSensor = new QLightSensor(this);
    mAutoBrightnessSettings  = new QGSettings(SETTINGS_AUTO_BRIGHTNESS_SCHEMAS);
}

AutoBrightnessManager::~AutoBrightnessManager()
{
    if(mAutoBrightnessManager)
        delete mAutoBrightnessManager;
    if(mSensor)
        delete mSensor;
    if(mAutoBrightnessSettings)
        delete mAutoBrightnessSettings;
    if(m_currentRunLocalThread)
    {
        m_currentRunLocalThread->stopImmediately();
        m_currentRunLocalThread = NULL;
    }
}

AutoBrightnessManager *AutoBrightnessManager::AutoBrightnessManagerNew()
{
    if(nullptr == mAutoBrightnessManager)
        mAutoBrightnessManager = new AutoBrightnessManager();
    return mAutoBrightnessManager;
}

void AutoBrightnessManager::onLocalThreadDestroy(QObject* obj){
    if(qobject_cast<QObject*>(m_currentRunLocalThread) == obj)
        m_currentRunLocalThread = NULL;
}

void AutoBrightnessManager::AutoBrightnessUpdateState()
{

    QLightReading * lightReading = mSensor->reading();
    qreal lux = lightReading->lux();//获得具体光强

    if(qAbs(lux - mPreLux) <= 4.0)              // 如果传感器检测到的Lux值和上次值小于4.0Lux，则不进行亮度调整
        return;
    mPreLux = lux;
    if(m_currentRunLocalThread)                 // 如果已经存在了一个线程在调节亮度值，就终止该线程
    {
        qDebug() << "已经有线程在运行";
        m_currentRunLocalThread->stopImmediately();
        m_currentRunLocalThread = NULL;
    }
    double brightness_ac = 0;
    if(lux > 41.0){
        brightness_ac = 100.0;
    }
    else{
        brightness_ac = pow(lux, 1.2) + 13;
        brightness_ac = round(brightness_ac);
    }


    BrightThread* thread = new BrightThread(NULL, brightness_ac);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);         // 线程结束后调用deleteLater来销毁分配的内存
    connect(thread, &QObject::destroyed, this, &AutoBrightnessManager::onLocalThreadDestroy);
    thread->start();
    m_currentRunLocalThread = thread;
}

void AutoBrightnessManager::AutoBrightnessRefresh()
{
    if(mSensor->isActive()){

        AutoBrightnessUpdateState();
    } else {

    }
}

void AutoBrightnessManager::SetEnabled(bool enabled)
{

    if(mEnabled == enabled)
        return;
    mEnabled = enabled;
    if (mEnabled) {
        mSensor->start();

        AutoBrightnessRefresh();
    } else {
        if(m_currentRunLocalThread)
        {
            m_currentRunLocalThread->stopImmediately();
            m_currentRunLocalThread = NULL;
        }
        mSensor->stop();
        mPreLux = -4.f;
    }
}

void AutoBrightnessManager::AutoBrightnessSettingsChanged(QString key)
{
    bool autobright;
    autobright = mAutoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();

    if (key == AUTO_BRIGHTNESS_KEY) {
          SetEnabled(autobright);
    }

}

bool AutoBrightnessManager::AutoBrightnessManagerStart()
{
    qDebug("AutoBrightnessManager Start");
    bool autobright;
    autobright = mAutoBrightnessSettings->get(AUTO_BRIGHTNESS_KEY).toBool();
    connect(mSensor,SIGNAL(readingChanged()),this,SLOT(AutoBrightnessUpdateState()));
    connect(mSensor,SIGNAL(activeChanged()),this,SLOT(AutoBrightnessRefresh()));
    connect(mAutoBrightnessSettings,SIGNAL(changed(QString)),this,SLOT(AutoBrightnessSettingsChanged(QString)));
    SetEnabled(autobright);
    return true;
}

void AutoBrightnessManager::AutoBrightnessManagerStop()
{
    SetEnabled(false);
    qDebug("AutoBrightness Manager stop");
}
