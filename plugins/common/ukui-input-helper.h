/* -*- Mode: C; indent-tabs-mode: nil; tab-width: 4 -*-
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
#ifndef __USD_INPUT_HELPER_H
#define __USD_INPUT_HELPER_H

#include <glib.h>

#include <X11/extensions/XInput.h>
#include <X11/extensions/XIproto.h>

gboolean  supports_xinput_devices (void);
XDevice  *device_is_touchpad      (XDeviceInfo *deviceinfo);
gboolean  touchpad_is_present     (void);



#endif /* __USD_INPUT_HELPER_H */
