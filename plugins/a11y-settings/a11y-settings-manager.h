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
#ifndef A11YSETTINGS_H
#define A11YSETTINGS_H
#include <QApplication>
#include <QObject>
#include <QDebug>
#include <QGSettings/qgsettings.h>

class  A11ySettingsManager : public QObject
{
    Q_OBJECT

private:
    A11ySettingsManager();

public:
    ~A11ySettingsManager();
    static A11ySettingsManager* A11ySettingsManagerNew();
    bool A11ySettingsManagerStart();
    void A11ySettingsMAnagerStop();

public Q_SLOTS:
    void AppsSettingsChanged(QString);

private:
    static A11ySettingsManager* mA11ySettingsManager;
    QGSettings *interface_settings;
    QGSettings *a11y_apps_settings;
};

#endif // A11YSETTINGS_H
