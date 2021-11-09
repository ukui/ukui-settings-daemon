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
#include "xEventMonitor.h"


xEventMonitor::xEventMonitor(QObject *parent) : QThread(parent)
{
    isPress = false;
    start(QThread::LowestPriority);
}

void xEventMonitor::run()
{
    Display* display = XOpenDisplay(0);

    if (display == 0) {
        USD_LOG(LOG_DEBUG, "unable to open display\n");
        return;
    }

    // Receive from ALL clients, including future clients.
    XRecordClientSpec clients = XRecordAllClients;
    XRecordRange* range = XRecordAllocRange();
    if (range == 0) {
         USD_LOG(LOG_DEBUG,"unable to allocate XRecordRange\n");
        return;
    }

    // Receive KeyPress, KeyRelease, ButtonPress, ButtonRelease and MotionNotify events.
    memset(range, 0, sizeof(XRecordRange));
    range->device_events.first = KeyPress;
    range->device_events.last  = MotionNotify;

    // And create the XRECORD context.
    XRecordContext context = XRecordCreateContext(display, 0, &clients, 1, &range, 1);
    if (context == 0) {
        USD_LOG(LOG_DEBUG,"XRecordCreateContext failed\n");//fprintf(stderr, "XRecordCreateContext failed\n");
        return;
    }
    XFree(range);

    XSync(display, True);

    Display* display_datalink = XOpenDisplay(0);
    if (display_datalink == 0) {
//        fprintf(stderr, "unable to open second display\n");
        USD_LOG(LOG_DEBUG,"unable to open second display\n");
        return;
    }

    if (!XRecordEnableContext(display_datalink,                  context,  callback, (XPointer) this)) {
//        fprintf(stderr, "XRecordEnableContext() failed\n");
        USD_LOG(LOG_DEBUG,"XRecordEnableContext() failed\n");
        return;
    }

    XCloseDisplay(display);
    XCloseDisplay(display_datalink);
}

void xEventMonitor::callback(XPointer ptr, XRecordInterceptData* data)
{
    ((xEventMonitor *) ptr)->handleRecordEvent(data);
}

void xEventMonitor::handleRecordEvent(XRecordInterceptData* data)
{
    int eventKeysym;
    static int Bpress = 0;
    if (data->category == XRecordFromServer) {
        xEvent * event = (xEvent *)data->data;

        if (event->u.u.type!=KeyPress && event->u.u.type!=KeyRelease){
            goto ERR;
        }

        switch (event->u.u.type) {
        case KeyPress:
            switch(event->u.u.detail){
            case 0x32:
                shiftPress_l = true;
                break;
            case 0x25:
                ctrlPress_l = true;
                break;
            case 0x40:
                altPress_l = true;
                break;
            case 0x3e:
                shiftPress_r = true;
                break;
            case 0x6c:
                altPress_r = true;
                break;
            case 0x69:
                 ctrlPress_r = true;
                break;
             case 0x85:
                 winPress = true;
                break;
             default :
                 Q_EMIT keyPress(event);
                break;
            }
            break;
        case KeyRelease:
            switch(event->u.u.detail){
            case 0x32:
                shiftPress_l = false;
                break;
            case 0x25:
                ctrlPress_l = false;
                break;
            case 0x40:
                altPress_l = false;
                break;
            case 0x3e:
                shiftPress_r = false;
                break;
            case 0x6c:
                altPress_r = false;
                break;
            case 0x69:
                ctrlPress_r = false;
                break;
            case 0x85:
                winPress = false;
                break;
            default :
                 Q_EMIT keyRelease(event);
                break;
            }
            break;
        default:
            break;
        }
    }
ERR:
    XRecordFreeData(data);
}

bool xEventMonitor::filterWheelEvent(int detail)
{
    return detail != WheelUp && detail != WheelDown && detail != WheelLeft && detail != WheelRight;
}
