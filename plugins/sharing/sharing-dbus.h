/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2021 KylinSoft Co., Ltd.
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

#ifndef SHARINGDBUS_H
#define SHARINGDBUS_H

#include <QObject>

class sharingDbus : public QObject
{
    Q_OBJECT
        Q_CLASSINFO("D-Bus Interface","org.ukui.SettingsDaemon.Sharing")

public:
    sharingDbus(QObject* parent=0);
    ~sharingDbus();

public Q_SLOTS:
    void EnableService(QString serviceName);
    void DisableService(QString serviceName);

Q_SIGNALS:
    void serviceChange(QString state, QString serviceName);

};

#endif // SHARINGDBUS_H
