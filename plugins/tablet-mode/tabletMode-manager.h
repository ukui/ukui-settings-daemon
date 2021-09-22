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
#ifndef TABLETMODEMANAGET_H
#define TABLETMODEMANAGET_H

#include <QObject>
#include <QOrientationSensor>
#include <QOrientationReading>
#include <KSharedConfig>
#include <QGSettings/qgsettings.h>
#include <QDBusInterface>

class TabletModeManager : public QObject
{
    Q_OBJECT
private:
    TabletModeManager();
    TabletModeManager(TabletModeManager&)=delete;
    TabletModeManager&operator=(const TabletModeManager&)=delete;

public:
    ~TabletModeManager();
    static TabletModeManager *TabletModeManagerNew();
    bool TabletModeManagerStart();
    void TabletModeManagerStop();
    void SetEnabled(bool enabled);

    void setConfig(KSharedConfig::Ptr config) {
        mConfig = std::move(config);
    }


public Q_SLOTS:
    void TabletUpdateState();
    void TabletRefresh();
    void TabletSettingsChanged(const bool tablemode);

private:
    QDBusInterface *t_DbusTableMode;
    bool   mEnabled = false;
    QGSettings   *mXrandrSettings;
    QGSettings   *mTableSettings;
    static TabletModeManager *mTabletManager;
    QOrientationSensor       *mSensor;
    KSharedConfig::Ptr        mConfig;
};

#endif // TABLETMODEMANAGET_H
