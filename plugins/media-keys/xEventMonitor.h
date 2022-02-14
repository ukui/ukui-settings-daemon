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


#ifndef XEVENTMONITOR_H
#define XEVENTMONITOR_H

#include <QThread>
#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/extensions/record.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <QX11Info>
// Virtual button codes that are not defined by X11.
#define Button1			1
#define Button2			2
#define Button3			3
#define WheelUp			4
#define WheelDown		5
#define WheelLeft		6
#define WheelRight		7
#define XButton1		8
#define XButton2		9

extern "C"{
#include "clib-syslog.h"
#include "usd_global_define.h"
}


class xEventMonitor : public QThread
{
    Q_OBJECT

public:
    xEventMonitor(QObject *parent = 0);
    bool getWinPressStatus();
    bool getCtrlPressStatus();
    bool getAltPressStatus();
    bool getShiftPressStatus();

Q_SIGNALS:
    void buttonPress(int x, int y);
    void buttonDrag(int x, int y);
    void buttonRelease(int x, int y);

    void keyPress(xEvent *code);
    void keyRelease(xEvent *code);

protected:
    bool filterWheelEvent(int detail);
    static void callback(XPointer trash, XRecordInterceptData* data);
    void handleRecordEvent(XRecordInterceptData *);
    void run();

private:
    bool winPress=false;
    bool ctrlPress_l=false;
    bool altPress_l=false;
    bool shiftPress_l = false;


    bool ctrlPress_r=false;
    bool altPress_r=false;
    bool shiftPress_r = false;


    bool isPress;
};

#endif // XEVENTMONITOR_H

