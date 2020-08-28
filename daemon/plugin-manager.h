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
#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "global.h"
#include "plugin-info.h"

#include <QList>
#include <QString>
#include <QObject>
#include <QApplication>
#include <QDBusConnection>

namespace UkuiSettingsDaemon {
class PluginManager;
}

class PluginManager : QObject
{
    Q_OBJECT
    Q_CLASSINFO ("D-Bus Interface", UKUI_SETTINGS_DAEMON_DBUS_NAME)

public:
    ~PluginManager();
    static PluginManager* getInstance();

private:
    PluginManager();
    PluginManager(PluginManager&)=delete;
    PluginManager& operator= (const PluginManager&)=delete;

Q_SIGNALS:
    void exit ();

public Q_SLOTS:
    void managerStop ();
    bool managerStart ();
    bool managerAwake ();

private:
    static QList<PluginInfo*>*      mPlugin;
    static PluginManager*           mPluginManager;
};

#endif // PLUGIN_MANAGER_H
