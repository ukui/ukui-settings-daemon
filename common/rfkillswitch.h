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

#ifndef RFKILLSWITCH_H
#define RFKILLSWITCH_H

#include <QObject>
#include <QFile>
#include <QTextStream>

extern "C" {

#include <linux/types.h>
#include <fcntl.h>
#include <unistd.h>

#define RFKILL_EVENT_SIZE_V1	8

#define VIRTUAL_PATH "/sys/devices/virtual/ieee80211"

const char * getRFkillType(__u32 idx);
const char * getRFkillName(__u32 idx);

struct rfkill_event {
    __u32 idx;
    __u8  type;
    __u8  op;
    __u8  soft, hard;
};

enum rfkill_operation {
    RFKILL_OP_ADD = 0,
    RFKILL_OP_DEL,
    RFKILL_OP_CHANGE,
    RFKILL_OP_CHANGE_ALL,
};

enum rfkill_type {
    RFKILL_TYPE_ALL = 0,
    RFKILL_TYPE_WLAN,
    RFKILL_TYPE_BLUETOOTH,
    RFKILL_TYPE_UWB,
    RFKILL_TYPE_WIMAX,
    RFKILL_TYPE_WWAN,
    RFKILL_TYPE_GPS,
    RFKILL_TYPE_FM,
    RFKILL_TYPE_NFC,
    NUM_RFKILL_TYPES,
};

}
/* 无线设备软开关 */
class RfkillSwitch : public QObject
{
    Q_OBJECT
public:
    explicit RfkillSwitch(){}
    RfkillSwitch(const RfkillSwitch&){}
    RfkillSwitch& operator==(const RfkillSwitch&){}

    static RfkillSwitch* instance(){return m_rfkillInstance;}
public:
    bool isVirtualWlan(QString dp);

    int getCurrentFlightMode();
    QString toggleFlightMode(bool enable);

    int getCurrentWlanMode();
    QString getWifiState();
    bool wifiDeviceIsPresent();
    void turnWifiOff();
    void turnWifiOn();

    QString getCameraBusinfo();
    int getCameraDeviceEnable();
    QString toggleCameraDevice(QString businfo);

    int getCurrentBluetoothMode();
    QString toggleBluetoothMode(bool enable);

private:
    static RfkillSwitch* m_rfkillInstance;


Q_SIGNALS:

};

#endif // RFKILLSWITCH_H
