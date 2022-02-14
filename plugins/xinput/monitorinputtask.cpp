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

#include "monitorinputtask.h"
#include "xinputmanager.h"

MonitorInputTask* MonitorInputTask::instance(QObject *parent)
{
    static MonitorInputTask *_instance = nullptr;
    QMutex mutex;
    mutex.lock();
    if(!_instance)
            _instance = new MonitorInputTask(parent);
    mutex.unlock();
    return _instance;
}

MonitorInputTask::MonitorInputTask(QObject *parent):
    QObject(parent),
    m_running(false)
{
    initConnect();
}

void MonitorInputTask::initConnect()
{

}

void MonitorInputTask::StartManager()
{
    USD_LOG(LOG_DEBUG,"info: [MonitorInputTask][StartManager]: thread id =%d",QThread::currentThreadId());
    QTimer::singleShot(0, this, &MonitorInputTask::ListeningToInputEvent);
}

int MonitorInputTask::EventSift(XIHierarchyEvent *event, int flag)
{
    int device_id = -1;
    int cnt = event->num_info;
    XIHierarchyInfo *event_list = event->info;
    for(int i = 0;i < cnt;i++)
    {
        if(event_list[i].flags & flag)
        {
            device_id = event_list[i].deviceid;
        }
    }
    return device_id;
}



void MonitorInputTask::ListeningToInputEvent()
{
    USD_LOG(LOG_DEBUG,"Start ListeningToInputEvent!");
    Display *display = NULL;
    // open display

//    PEEK_XOpenDisplay(display);
    display = XOpenDisplay(NULL);


   // display = XOpenDisplay(NULL);
//    openDisplayOK = true;
    //QX11Info::display();

    USD_LOG(LOG_DEBUG, "choke check pass......");

    if (display == NULL)
    {
        USD_LOG(LOG_ERR,"OpenDisplay fail....");
        return;
    }

    XIEventMask mask[2];
    XIEventMask *m;
    Window win;
     USD_LOG(LOG_DEBUG, "choke check pass......");
    win = DefaultRootWindow(display);
     USD_LOG(LOG_DEBUG, "choke check pass......");
    m = &mask[0];
    m->deviceid = XIAllDevices;
    m->mask_len = XIMaskLen(XI_LASTEVENT);
    m->mask = (unsigned char*)calloc(m->mask_len, sizeof(char));
    XISetMask(m->mask, XI_HierarchyChanged);
     USD_LOG(LOG_DEBUG, "choke check pass......");
    m = &mask[1];
    m->deviceid = XIAllMasterDevices;
    m->mask_len = XIMaskLen(XI_LASTEVENT);
    m->mask = (unsigned char*)calloc(m->mask_len, sizeof(char));

    XISelectEvents(display, win, &mask[0], 2);
    XSync(display, False);
    USD_LOG(LOG_DEBUG, "choke check pass......");
    free(mask[0].mask);
    free(mask[1].mask);

    while(true)
    {
        XEvent ev;
        XGenericEventCookie *cookie = (XGenericEventCookie*)&ev.xcookie;
        USD_LOG(LOG_DEBUG, "choke chdeck pass......cookie->evtype:%d",cookie->evtype);

//        if (cookie->type != GenericEvent){
//            USD_LOG(LOG_DEBUG, "choke chdeck pass......type error:%d is't GenericEvent(%d)",cookie->evtype,GenericEvent);
//            continue;
//        }

        XNextEvent(display, (XEvent*)&ev);//该方法会引起glib崩溃一次.下次出现记录具体的事件类型，该方法是阻塞处理。。。。
        USD_LOG(LOG_DEBUG, "choke check pass......event:%d",ev.type);
        // 判断当前事件监听是否还要继续
        // 保证效率 m_running[*bool] 的访问不需要加锁
        if(!m_running){
            USD_LOG(LOG_DEBUG, "choke check pass......break");
            break;
        }
        USD_LOG(LOG_DEBUG, "choke check pass......");
        if (XGetEventData(display, cookie) &&
            cookie->type == GenericEvent)
        {
             USD_LOG(LOG_DEBUG, "choke check pass......");
            if(XI_HierarchyChanged == cookie->evtype)
            {
                XIHierarchyEvent *hev = (XIHierarchyEvent*)cookie->data;
                if(hev->flags & XIMasterRemoved)
                {
                    Q_EMIT masterRemoved(EventSift(hev, XIMasterRemoved));
                }
                else if(hev->flags & XISlaveAdded)
                {
                    Q_EMIT slaveAdded(EventSift(hev, XISlaveAdded));
                }
                else if(hev->flags & XISlaveRemoved)
                {
                    Q_EMIT slaveRemoved(EventSift(hev, XISlaveRemoved));
                }
                else if(hev->flags & XISlaveAttached)
                {
                    Q_EMIT slaveAttached(EventSift(hev, XISlaveAttached));
                }
                else if(hev->flags & XISlaveDetached)
                {
                    Q_EMIT slaveDetached(EventSift(hev, XISlaveDetached));
                }
                else if(hev->flags & XIDeviceEnabled)
                {
                    Q_EMIT deviceEnable(EventSift(hev, XIDeviceEnabled));
                }
                else if(hev->flags & XIDeviceDisabled)
                {
                    Q_EMIT deviceDisable(EventSift(hev, XIDeviceDisabled));
                }
                else if(hev->flags & XIMasterAdded)
                {
                    Q_EMIT masterAdded(EventSift(hev, XIMasterAdded));
                }
            }
        }
        USD_LOG(LOG_DEBUG, "choke check pass......");
        XFreeEventData(display, cookie);
    }

    XDestroyWindow(display, win);
}
