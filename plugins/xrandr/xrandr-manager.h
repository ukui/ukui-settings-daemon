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
#ifndef XRANDRMANAGER_H
#define XRANDRMANAGER_H

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QDomDocument>
#include <QtXml>
#include <QX11Info>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QWidget>
#include <QMultiMap>
#include <QScreen>
#include <QGSettings/qgsettings.h>

#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/Xrandr.h>
#include <xorg/xserver-properties.h>
#include <gudev/gudev.h>

extern "C" {
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-rr.h>
#include <libmate-desktop/mate-rr-config.h>
#include <libmate-desktop/mate-rr-labeler.h>
#include <libmate-desktop/mate-desktop-utils.h>
}

class XrandrManager: public QObject
{
    Q_OBJECT
private:
    XrandrManager();
    XrandrManager(XrandrManager&)=delete;
    XrandrManager&operator=(const XrandrManager&)=delete;
public:
    ~XrandrManager();
    static XrandrManager *XrandrManagerNew();
    bool XrandrManagerStart();
    void XrandrManagerStop();

public Q_SLOTS:
    void StartXrandrIdleCb ();
    void RotationChangedEvent(QString);

public:
    bool ReadMonitorsXml();
    bool SetScreenSize(Display  *dpy, Window root, int width, int height);
    void RestoreBackupConfiguration(XrandrManager *manager,
                                    const char    *backup_filename,
                                    const char    *intended_filename,
                                    unsigned int   timestamp);
    static void OnRandrEvent (MateRRScreen *screen, gpointer data);
    static void AutoConfigureOutputs (XrandrManager *manager,
                                      unsigned int  timestamp);
    static bool ApplyConfigurationFromFilename   (XrandrManager *manager,
                                                  const char    *filename,
                                                  unsigned int   timestamp);
    static bool ApplyStoredConfigurationAtStartup(XrandrManager *manager,
                                                  unsigned int timestamp);
    static void monitorSettingsScreenScale (MateRRScreen *screen);
    static void oneScaleLogoutDialog(QGSettings *settings);
    static void twoScaleLogoutDialog(QGSettings *settings);

private:
    QTimer                *time;
    QGSettings            *mXrandrSetting;
    static XrandrManager  *mXrandrManager;
    MateRRScreen          *mScreen;

protected:
    QMultiMap<QString, QString> XmlFileTag; //存放标签的属性值
    QMultiMap<QString, int>     mIntDate;
};

#endif // XRANDRMANAGER_H
