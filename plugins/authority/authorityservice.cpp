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

#include "authorityservice.h"

extern "C" {

#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>
}

AuthorityService::AuthorityService(QObject *parent) : QObject(parent)
{

}

QString AuthorityService::getCameraBusinfo()
{

    QString path = QString("/sys/bus/usb/devices/");
    QDir dir(path);
    if (!dir.exists()){
        return QString();
    }
    dir.setFilter(QDir::Dirs);
    dir.setSorting(QDir::Name);
    QFileInfoList fileinfoList = dir.entryInfoList();

    for(QFileInfo fileinfo : fileinfoList){
        if (fileinfo.fileName() == "." || fileinfo.fileName() == ".."){
            continue;
        }

        if (fileinfo.fileName().contains(":")){
            continue;
        }

        if (fileinfo.fileName().startsWith("usb")){
            continue;
        }

        QDir subdir(fileinfo.absoluteFilePath());
        subdir.setFilter(QDir::Files);
        QFileInfoList fileinfoList2 = subdir.entryInfoList();
        for (QFileInfo fileinfo2 : fileinfoList2){
            if (fileinfo2.fileName() == "product"){
                QFile pfile(fileinfo2.absoluteFilePath());
                if (!pfile.open(QIODevice::ReadOnly | QIODevice::Text))
                    return QString();
                QTextStream pstream(&pfile);
                QString output = pstream.readAll();

                if (output.contains("camera", Qt::CaseInsensitive)){
                    return fileinfo.fileName();
                }

            }
        }

    }

    return QString();
}

int AuthorityService::getCameraDeviceEnable()
{

    QString businfo = getCameraBusinfo();
    if (businfo.isEmpty()){
        const char cmd[] = "lsusb -t | grep 'Driver=uvcvideo'";
        char output[1024] = "\0";

        FILE * stream;
        if ((stream = popen(cmd, "r")) == NULL){
            return -1;
        }

    int ret;
        if (fread(output, sizeof(char), 1024, stream) <= 0){
        ret = 0;
        } else {
        ret = 1;
        }
    fclose(stream);
    return ret;
    }

    int isExists = 0;

    QString path = QString("/sys/bus/usb/drivers/usb/");
    QDir dir(path);
    if (!dir.exists()){
        return -1;
    }
    dir.setFilter(QDir::Dirs);
    dir.setSorting(QDir::Name);
    QFileInfoList fileinfoList = dir.entryInfoList();

    for(QFileInfo fileinfo : fileinfoList){
        if (fileinfo.fileName() == "." || fileinfo.fileName() == ".."){
            continue;
        }

        if (fileinfo.fileName().contains(":")){
            continue;
        }

        if (fileinfo.fileName() == businfo){
            isExists = 1;
        }
    }

    return isExists;
}

QString AuthorityService::toggleCameraDevice()
{
    QString path = QString("/sys/bus/usb/drivers/usb/");

    int status = getCameraDeviceEnable();

    if (status == -1){
        return QString("Camera Device Not Exists!");
    }

    QString businfo = getCameraBusinfo();
    if (status){
        QString cmd = QString("echo '%1' > %2/unbind").arg(businfo).arg(path);
        system(cmd.toLatin1().data());
        return QString("unbinded");
    } else {
        QString cmd = QString("echo '%1' > %2/bind").arg(businfo).arg(path);
        system(cmd.toLatin1().data());
        return QString("binded");
    }
}

int AuthorityService::setCameraKeyboardLight(bool lightup)
{
    const char target[] = "/sys/class/leds/platform::cameramute/brightness";
    if (access(target, F_OK) == -1){
        return -1;
    }

    int value = lightup ? 1 : 0;

    char cmd[256];
    sprintf(cmd, "echo %d > %s", value, target);

    system(cmd);

    return 1;
}
