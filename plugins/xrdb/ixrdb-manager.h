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
#ifndef IXRDBMANAGER_H
#define IXRDBMANAGER_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "clib-syslog.h"

#ifdef __cplusplus
}
#endif

class IXrdbManager {
public:
    IXrdbManager(){};
    virtual ~IXrdbManager(){};
    virtual bool start(GError**) = 0;
    virtual void stop() = 0;
};

#endif // IXRDBMANAGER_H
