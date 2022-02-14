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

#ifndef USDBASECLASS_H
#define USDBASECLASS_H
#include <QObject>
#include <QMetaEnum>
#include <ukuisdk/kylin-com4cxx.h>

extern QString g_motify_poweroff;

class UsdBaseClass: public QObject
{
    Q_OBJECT

public:
    UsdBaseClass();
    ~UsdBaseClass();
    enum eScreenMode {
        firstScreenMode = 0,
        cloneScreenMode,
        extendScreenMode,
        secondScreenMode};

    Q_ENUM(eScreenMode)

    static bool isTablet();

    static bool isMasterSP1();

    static bool is9X0();

    static bool isWayland();

    static bool isXcb();

    static bool isNotebook();

    static bool isLoongarch();

    static bool isUseXEventAsShutKey();

    static bool readPowerOffConfig();

    static bool isPowerOff();
};

#endif // USDBASECLASS_H
