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
#ifndef MOUSEMANAGER_H
#define MOUSEMANAGER_H
#include <QApplication>
#include <QDebug>
#include <QObject>
#include <QTimer>
#include <QDir>
#include <QProcess>
#include <QtX11Extras/QX11Info>
#include <QGSettings/qgsettings.h>
#include <QDBusInterface>
#include <QDBusReply>

#include <glib.h>
#include <errno.h>
#include <math.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

class MouseManager : public QObject
{
    Q_OBJECT

private:
    MouseManager()=delete;
    MouseManager(MouseManager&)=delete;
    MouseManager&operator=(const MouseManager&)=delete;
    MouseManager(QObject *parent = nullptr);
public:
    ~MouseManager();
    static MouseManager * MouseManagerNew();
    bool MouseManagerStart();
    void MouseManagerStop();

public Q_SLOTS:
    void MouseManagerIdleCb();
    void MouseCallback(QString);
    void TouchpadCallback(QString);
    void initWaylandDbus();
    void initWaylandMouseStatus();

public:
    void SetLeftHandedAll (bool mouse_left_handed,
                           bool touchpad_left_handed);
    void SetLeftHanded  (XDeviceInfo  *device_info,
                         bool         mouse_left_handed,
                         bool         touchpad_left_handed);
    void SetLeftHandedLegacyDriver (XDeviceInfo     *device_info,
                                    bool         mouse_left_handed,
                                    bool         touchpad_left_handed);

    void SetMotionAll ();
    void SetMotion   (XDeviceInfo  *device_info);
    void SetMotionLibinput (XDeviceInfo  *device_info);
    void SetMotionLegacyDriver(XDeviceInfo     *device_info);
    bool GetTouchpadHandedness (bool mouse_left_handed);
    void SetMouseAccel(XDeviceInfo  *device_info);
    void SetTouchpadStateAll(int num, bool state);
    void SetTouchpadMotionAccel(XDeviceInfo *device_info);
    void SetBottomRightClickMenu (XDeviceInfo *device_info, bool state);
    void SetBottomRightConrnerClickMenu(bool state);

    void SetDisableWTyping  (bool state);
    void SetDisableWTypingSynaptics (bool state);
    void SetDisableWTypingLibinput  (bool state);

    void SetMiddleButtonAll   (bool middle_button);
    void SetMiddleButton      (XDeviceInfo *device_info,
                               bool     middle_button);
    void SetLocatePointer     (bool     state);
    void SetTapToClickAll ();
    void SetNaturalScrollAll ();
    void SetDevicepresenceHandler ();
    void SetMouseWheelSpeed (int speed);
    void SetMouseSettings();

private: 
    friend GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
                                                  GdkEvent  *event,
                                                  gpointer   data);

private:
    unsigned long mAreaLeft;
    unsigned long mAreaTop;
    QTimer * time;
    QGSettings *settings_mouse;
    QGSettings *settings_touchpad;
#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
    gboolean mousetweaks_daemon_running;
#endif
    gboolean syndaemon_spawned;
    GPid     syndaemon_pid;
    gboolean locate_pointer_spawned;
    GPid     locate_pointer_pid;
    bool     imwheelSpawned;
    bool     mDeviceFlag = false;
    QDBusInterface *mWaylandIface;
    QDBusInterface *mMouseDeviceIface;
    QDBusInterface *mTouchDeviceIface;

    static MouseManager *mMouseManager;
};

#endif // MOUSEMANAGER_H
