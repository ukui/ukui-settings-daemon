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


#ifndef KDSWIDGET_H
#define KDSWIDGET_H

#include <QObject>
#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/edid.h>
#include <KF5/KScreen/kscreen/mode.h>
#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/getconfigoperation.h>
#include <KF5/KScreen/kscreen/setconfigoperation.h>
#include "xrandr-config.h"


class SaveScreenParam :  public QObject
{
    Q_OBJECT

//    Q_PROPERTY(bool isSet READ isSet WRITE setIsSet )
//    Q_PROPERTY(bool isGet READ isGet WRITE setIsGet )

public:

    explicit SaveScreenParam(QObject *parent = nullptr) ;
    ~SaveScreenParam();
    void setIsSet(bool value)
    {
        m_isSet = value;
    }

    void setIsGet(bool value)
    {
        m_isGet = value;
    }

    bool isSet() const
    {
        return m_isSet;
    }

    bool isGet() const
    {
        return m_isGet;
    }

    void getConfig();

    void setUserName(QString str);

    void readConfigAndSet();
private:
    bool m_isSet;
    bool m_isGet;

    QString m_userName;

    std::unique_ptr<xrandrConfig> m_MonitoredConfig = nullptr;
};

#endif // KDSWIDGET_H
