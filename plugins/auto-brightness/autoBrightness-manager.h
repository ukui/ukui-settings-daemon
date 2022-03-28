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
#include <QDBusInterface>
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
    static AutoBrightnessManager *autoBrightnessManagerNew();
    bool autoBrightnessManagerStart();
    void autoBrightnessManagerStop();


public Q_SLOTS:
    void sensorReadingChangedSlot();
    void sensorActiveChangedSlot();
    void gsettingsChangedSlot(QString key);
    void idleModeChangeSlot(quint32 mode);
private:
    void setEnabled(bool enabled);
    void enableSensorAndSetGsettings(bool state);
    void adjustBrightnessWithLux(qreal lux);
private:
    bool m_enableAutoBrightness;
    bool m_hadSensor;

    bool m_Enabled = false;

    QGSettings                      *m_AutoBrightnessSettings;
    QLightSensor                    *m_Sensor;
    KSharedConfig::Ptr               m_Config;
    BrightThread                    *m_brightnessThread;
    static AutoBrightnessManager    *m_AutoBrightnessManager;
};

#endif // AUTOBRIGHTNESSMANAGER_H
