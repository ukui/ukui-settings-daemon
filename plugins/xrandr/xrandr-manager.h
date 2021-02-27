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
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QGSettings/qgsettings.h>
#include <QScreen>

#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/log.h>
#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/edid.h>
#include <KF5/KScreen/kscreen/configmonitor.h>
#include <KF5/KScreen/kscreen/getconfigoperation.h>
#include <KF5/KScreen/kscreen/setconfigoperation.h>

#include <QOrientationReading>
#include <memory>
#include "xrandr-dbus.h"
#include "xrandr-adaptor.h"
#include "xrandr-config.h"

#include <glib.h>
#include <X11/Xlib.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XInput.h>
#include <X11/extensions/Xrandr.h>
#include <xorg/xserver-properties.h>
#include <gudev/gudev.h>

class XrandrManager: public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KScreen")

public:
    XrandrManager();
    ~XrandrManager() override;

public:
    bool XrandrManagerStart();
    void XrandrManagerStop();
    void StartXrandrIdleCb ();
    void monitorsInit();
    void applyConfig();
    void applyKnownConfig();
    void applyIdealConfig();
    void outputConnectedChanged();
    void doApplyConfig(const KScreen::ConfigPtr &config);
    void doApplyConfig(std::unique_ptr<xrandrConfig> config);
    void refreshConfig();

    void configChanged();
    void saveCurrentConfig();
    void setMonitorForChanges(bool enabled);

    void outputAdded(const KScreen::OutputPtr &output);
    void outputRemoved(int outputId);
    void primaryOutputChanged(const KScreen::OutputPtr &output);
    void orientationChangedProcess(Qt::ScreenOrientation orientation);
    void init_primary_screens(KScreen::ConfigPtr config);

    void callMethod(QRect geometry, QString name);

public Q_SLOTS:
    void RotationChangedEvent(QString);

Q_SIGNALS:
    // DBus
    void outputConnected(const QString &outputName);
    void unknownOutputConnected(const QString &outputName);

private:
    Q_INVOKABLE void getInitialConfig();
    QTimer                *time;
    QTimer                *mSaveTimer;
    QTimer                *mChangeCompressor;
    QGSettings            *mXrandrSetting;
    int                   mScale = 1;
    std::unique_ptr<xrandrConfig> mMonitoredConfig;
    KScreen::ConfigPtr mConfig;
    bool mMonitoring;
    bool mConfigDirty = true;
    QScreen *mScreen;
    bool mStartingUp = true;
};

#endif // XRANDRMANAGER_H
