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

#include "sharing-dbus.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>

sharingDbus::sharingDbus(QObject *parent) : QObject(parent)
{

}

sharingDbus::~sharingDbus()
{

}

void sharingDbus::EnableService(QString serviceName)
{
    qDebug()<<__func__<<serviceName;
    Q_EMIT serviceChange("enable", serviceName);
}

void sharingDbus::DisableService(QString serviceName)
{
    qDebug()<<__func__<<serviceName;
    Q_EMIT serviceChange("disable", serviceName);
}
