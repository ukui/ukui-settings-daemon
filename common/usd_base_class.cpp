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

#include <QX11Info>
#include <QGuiApplication>
#include "clib-syslog.h"
#include "usd_base_class.h"


#include <QDBusMessage>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDebug>
#include <QFile>
#include <QDir>

#define STR_EQUAL 0

#define DBUS_SERVICE "org.freedesktop.UPower"
#define DBUS_OBJECT "/org/freedesktop/UPower"
#define DBUS_INTERFACE "org.freedesktop.DBus.Properties"
#define POWER_OFF_CONFIG_FILE "/sys/class/dmi/id/modalias"

QString g_motify_poweroff;

UsdBaseClass::UsdBaseClass()
{
}

UsdBaseClass::~UsdBaseClass()
{

}

bool UsdBaseClass::isMasterSP1()
{
    return false;
}

bool UsdBaseClass::isUseXEventAsShutKey()
{
    return true;
}

bool UsdBaseClass::isTablet()
{
    static QString projectCode = nullptr;
    QString deviceName = "v10sp1-edu";

    if (nullptr == projectCode) {
        QString arg = KDKGetPrjCodeName().c_str();
        projectCode = arg.toLower();
    }
    if (QString::compare(projectCode,deviceName) == STR_EQUAL) {
        return true;
    } else {
        return false;
    }
}

bool UsdBaseClass::isLoongarch()
{
    QString cpuMode = KDKGetCpuModelName().c_str();
    USD_LOG(LOG_DEBUG,"GetCpuModelName : %s",cpuMode.toStdString().c_str());
    if(cpuMode.toLower().contains("loongson")){
        return true;
    }
    return false;
}

bool UsdBaseClass::is9X0()
{
#ifdef USD_9X0
     return true;
#endif
    return false;
}

bool UsdBaseClass::isWayland()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        USD_LOG(LOG_DEBUG,"is wayland app");
        return true;
    }

    USD_LOG(LOG_DEBUG,"is xcb app");
    return false;
}

bool UsdBaseClass::isXcb()
{
    if (QGuiApplication::platformName().startsWith(QLatin1String("xcb"))) {
        USD_LOG(LOG_DEBUG,"is xcb app");
        return true;
    }
    return false;
}

bool UsdBaseClass::isNotebook()
{
    QDBusMessage msg = QDBusMessage::createMethodCall(DBUS_SERVICE,DBUS_OBJECT,DBUS_INTERFACE,"Get");
    msg<<DBUS_SERVICE<<"LidIsPresent";
    QDBusMessage res = QDBusConnection::systemBus().call(msg);
    if(res.type()==QDBusMessage::ReplyMessage)
    {
        QVariant v = res.arguments().at(0);
        QDBusVariant dv = qvariant_cast<QDBusVariant>(v);
        QVariant result = dv.variant();
        return result.toBool();
    }

    return false;
}

bool UsdBaseClass::readPowerOffConfig()
{
    QDir dir;
    QFile file;
    QString filePath = POWER_OFF_CONFIG_FILE;

    file.setFileName(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        false;

    QTextStream pstream(&file);
    g_motify_poweroff = pstream.readAll();
    file.close();
    return true;
}

bool UsdBaseClass::isPowerOff()
{
    const QStringList devName ={"pnPF215T"};

    if(g_motify_poweroff.isEmpty())
        readPowerOffConfig();

    for(auto devNameTmp : devName)
    {
        if (g_motify_poweroff.contains(devNameTmp, Qt::CaseSensitive))
        {
            return true;
        }
    }
    return false;
}

bool UsdBaseClass::isTrialModeByPopen()
{
    int ret = 0xff;
    char Cmd[512] = {0};
    char *pAck = NULL;
    char CmdAck[120];
    FILE * pPipe;

    if (ret!=0xff) {
        return ret;
    }

    pPipe = popen("cat /prop/cmdline","r");

    if (pPipe) {
        pAck = fgets(CmdAck,sizeof(CmdAck),pPipe);
        if (strstr(CmdAck, "boot=casper") > 0) {
            ret = true;
        } else{
            ret = false;
        }

        pclose(pPipe);
    }

    return ret;
}
