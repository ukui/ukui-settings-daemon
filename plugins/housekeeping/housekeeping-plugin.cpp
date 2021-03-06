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
#include <QProcess>
#include "housekeeping-plugin.h"
#include "clib-syslog.h"

PluginInterface     *HousekeepingPlugin::mInstance=nullptr;

QString getCurrentUserName()
{
    QString name;
    if (name.isEmpty()){
        QStringList envList = QProcess::systemEnvironment();
        for(const QString& env : envList){
            if(env.startsWith("USERNAME")){
                QStringList strList = env.split('=');
                if(strList.size() > 2){
                    name = strList[1];
                }
            }
        }
    }
    if(!name.isEmpty())
        return name;
    QProcess process;
    process.start("whoami", QStringList());
    process.waitForFinished();
    name = QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
    return name.isEmpty() ? QString("User") : name;
}

HousekeepingPlugin::HousekeepingPlugin()
{
    userName = getCurrentUserName();
    if(userName.compare("lightdm") != 0){
        mHouseManager = new HousekeepingManager();
        if(!mHouseManager)
            syslog(LOG_ERR,"Unable to start Housekeeping Manager!");
    }
}

HousekeepingPlugin::~HousekeepingPlugin()
{
    if(mHouseManager){
        delete mHouseManager;
        mHouseManager = nullptr;
    }
}

void HousekeepingPlugin::activate()
{
    char str[1024];
    FILE *fp;
    fp = popen("cat /proc/cmdline", "r");
    while(fgets(str, sizeof(str)-1, fp))
    {
        if(strstr(str,"boot=casper")){
            printf("is livecd\n");
            pclose(fp);
            return;
        }
    }
    pclose(fp);
    if(getuid() == 999)
          return;

    if(userName.compare("lightdm") != 0){
        mHouseManager->HousekeepingManagerStart();
    }
}

PluginInterface *HousekeepingPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new HousekeepingPlugin();
    return mInstance;
}

void HousekeepingPlugin::deactivate()
{
    if(mHouseManager)
        mHouseManager->HousekeepingManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return HousekeepingPlugin::getInstance();
}
