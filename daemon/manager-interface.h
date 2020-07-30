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
#ifndef MANAGER_INTERFACE_H
#define MANAGER_INTERFACE_H

#include "plugin-manager.h"

#include <QMap>
#include <QList>
#include <QObject>
#include <QString>
#include <QtDBus>
#include <QVariant>
#include <QByteArray>
#include <QStringList>

namespace UkuiSettingsDaemon {
class PluginManagerDBus;
}

/*
 * Proxy class for interface org.ukui.SettingsDaemon
 */
class PluginManagerDBus: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "org.ukui.SettingsDaemon"; }

public:
    PluginManagerDBus(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    ~PluginManagerDBus();

public Q_SLOTS:
    inline QDBusPendingReply<bool> managerAwake()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("managerAwake"), argumentList);
    }

    inline QDBusPendingReply<bool> managerStart()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("managerStart"), argumentList);
    }

    inline QDBusPendingReply<> managerStop()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("managerStop"), argumentList);
    }

    inline QDBusPendingReply<QString> onPluginActivated()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("onPluginActivated"), argumentList);
    }

    inline QDBusPendingReply<QString> onPluginDeactivated()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("onPluginDeactivated"), argumentList);
    }
};

#endif
