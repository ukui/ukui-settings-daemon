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

#include <QString>
#include <QFile>
#include "housekeeping-plugin.h"
#include "clib-syslog.h"

PluginInterface     *HousekeepingPlugin::mInstance=nullptr;

HousekeepingPlugin::HousekeepingPlugin()
{
    mHouseManager = new HousekeepingManager();
    if(!mHouseManager)
        syslog(LOG_ERR,"Unable to start Housekeeping Manager!");
}

HousekeepingPlugin::~HousekeepingPlugin()
{
    if(mHouseManager){
        delete mHouseManager;
        mHouseManager = nullptr;
    }
}

bool HousekeepingPlugin::isTrialMode()
{
    QString str;
    QByteArray t;
    QFile file("/proc/cmdline");
    file.open(QIODevice::ReadOnly | QIODevice::Text);
    t = file.readAll();
    str = QString(t);
    if(str.indexOf("boot=casper") != -1){
        printf("is Trial Mode\n");
        file.close();
        return true;
    }
    file.close();
    if(getuid() == 999)
        return true;
    return false;
}

void HousekeepingPlugin::activate()
{
    if(isTrialMode())
        return;
    mHouseManager->HousekeepingManagerStart();
}

PluginInterface *HousekeepingPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new HousekeepingPlugin();
    return mInstance;
}

void HousekeepingPlugin::deactivate()
{
    if(isTrialMode())
        return;
    mHouseManager->HousekeepingManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return HousekeepingPlugin::getInstance();
}
