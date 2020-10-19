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

public:
    bool GetTouchpadHandedness (bool mouse_left_handed);
    void SetMouseAccel(XDeviceInfo  *device_info);
private:
    friend void set_left_handed_all     (MouseManager *manager,
                                         bool mouse_left_handed,
                                         bool touchpad_left_handed);
    friend void set_left_handed         (MouseManager *manager,
                                         XDeviceInfo     *device_info,
                                         bool         mouse_left_handed,
                                         bool         touchpad_left_handed);

    friend void set_left_handed_legacy_driver (MouseManager *manager,
                                               XDeviceInfo     *device_info,
                                               bool         mouse_left_handed,
                                               bool         touchpad_left_handed);

    friend void set_motion_all          (MouseManager *manager);
    friend void set_motion              (MouseManager *manager,
                                         XDeviceInfo  *device_info);
    friend void set_motion_libinput     (MouseManager *manager,
                                         XDeviceInfo  *device_info);
    friend void set_motion_legacy_driver(MouseManager *manager,
                                         XDeviceInfo     *device_info);
    friend void set_middle_button_all   (bool middle_button);
    friend void set_middle_button       (XDeviceInfo *device_info,
                                         bool     middle_button);
    friend void set_locate_pointer      (MouseManager *manager, bool     state);
    friend void set_disable_w_typing    (MouseManager *manager,
                                         bool         state);
    friend void set_disable_w_typing_synaptics (MouseManager *manager,
                                                bool         state);
    friend void set_disable_w_typing_libinput  (MouseManager *manager,
                                                bool         state);
    friend void set_tap_to_click_all        (MouseManager *manager);
    friend void set_natural_scroll_all      (MouseManager *manager);
    friend void set_devicepresence_handler  (MouseManager *manager);
    friend GdkFilterReturn devicepresence_filter (GdkXEvent *xevent,
                                                  GdkEvent  *event,
                                                  gpointer   data);
    friend void set_mouse_settings (MouseManager *manager);
    friend void set_mouse_wheel_speed (MouseManager *manager, int speed);

private:
    QTimer * time;
    QGSettings *settings_mouse;
    QGSettings *settings_touchpad;
    static MouseManager *mMouseManager;

#if 0   /* FIXME need to fork (?) mousetweaks for this to work */
    gboolean mousetweaks_daemon_running;
#endif
    gboolean syndaemon_spawned;
    GPid     syndaemon_pid;
    gboolean locate_pointer_spawned;
    GPid     locate_pointer_pid;
    bool     imwheelSpawned;

};

#endif // MOUSEMANAGER_H
