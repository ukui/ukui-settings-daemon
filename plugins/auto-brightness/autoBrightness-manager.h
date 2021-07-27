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
#ifndef AUTOBRIGHTNESSMANAGER_H
#define AUTOBRIGHTNESSMANAGER_H

#include <QObject>
#include <QLightSensor>
#include <QLightReading>
#include <KSharedConfig>
#include <QGSettings/qgsettings.h>
#include "brightThread.h"

class AutoBrightnessManager : public QObject
{
    Q_OBJECT
private:
    AutoBrightnessManager();
    AutoBrightnessManager(AutoBrightnessManager&)=delete;
    AutoBrightnessManager&operator=(const AutoBrightnessManager&)=delete;

public:
    ~AutoBrightnessManager();
    static AutoBrightnessManager *AutoBrightnessManagerNew();
    bool AutoBrightnessManagerStart();
    void AutoBrightnessManagerStop();
    void SetEnabled(bool enabled);

public Q_SLOTS:
    void AutoBrightnessUpdateState();
    void onLocalThreadDestroy(QObject* obj);
    void AutoBrightnessRefresh();
    void AutoBrightnessSettingsChanged(QString);

private:
    bool mEnabled;
    double mPreLux;
    QGSettings   *mAutoBrightnessSettings;
    static AutoBrightnessManager *mAutoBrightnessManager;
    QLightSensor       *mSensor;
    KSharedConfig::Ptr        mConfig;
    BrightThread * m_currentRunLocalThread;
};

#endif // AUTOBRIGHTNESSMANAGER_H
