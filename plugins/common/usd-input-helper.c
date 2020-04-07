/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#include "syslog.h"
#include "config.h"

#include <gdk/gdk.h>
#include <gdk/gdkx.h>

#include <sys/types.h>
#include <X11/Xatom.h>

#include "usd-input-helper.h"

gboolean
supports_xinput_devices (void)
{
        gint op_code, event, error;

        return XQueryExtension (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                "XInputExtension",
                                &op_code,
                                &event,
                                &error);
}

static gboolean
device_has_property (XDevice    *device,
                     const char *property_name)
{
        Atom realtype, prop;
        int realformat;
        unsigned long nitems, bytes_after;
        unsigned char *data;

        prop = XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), property_name, True);
        if (!prop)
                return FALSE;

        gdk_error_trap_push ();
        if ((XGetDeviceProperty (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device, prop, 0, 1, False,
                                XA_INTEGER, &realtype, &realformat, &nitems,
                                &bytes_after, &data) == Success) && (realtype != None)) {
                gdk_error_trap_pop_ignored ();
                XFree (data);
                return TRUE;
        }

        gdk_error_trap_pop_ignored ();
        return FALSE;
}

XDevice*
device_is_touchpad (XDeviceInfo *deviceinfo)
{
        XDevice *device;

        if (deviceinfo->type != XInternAtom (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), XI_TOUCHPAD, True)) {
		if (deviceinfo->type == 0) {
			//syslog(LOG_INFO, "deviceinfo type is NULL");
		} else {
			/*syslog(LOG_INFO,
                               "deviceinfo type : %s vs %s device_is_touchpad failed",
                               XGetAtomName(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), deviceinfo->type),
                               XGetAtomName(GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()),
                                            XInternAtom (GDK_DISPLAY_XDISPLAY(gdk_display_get_default ()), XI_TOUCHPAD, True))
                              );
                // return NULL;*/
		}
	}

        gdk_error_trap_push ();
        device = XOpenDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), deviceinfo->id);
        if (gdk_error_trap_pop () || (device == NULL)) {
                return NULL;
	}

        if (device_has_property (device, "libinput Tapping Enabled") ||
            device_has_property (device, "Synaptics Off")) {
                return device;
        }

        XCloseDevice (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), device);
        return NULL;
}

gboolean
touchpad_is_present (void)
{
        XDeviceInfo *device_info;
        gint n_devices;
        guint i;
        gboolean retval;

        if (supports_xinput_devices () == FALSE)
                return TRUE;

        retval = FALSE;

        device_info = XListInputDevices (GDK_DISPLAY_XDISPLAY (gdk_display_get_default ()), &n_devices);
        if (device_info == NULL)
                return FALSE;

        for (i = 0; i < n_devices; i++) {
                XDevice *device;

                device = device_is_touchpad (&device_info[i]);
                if (device != NULL) {
                        retval = TRUE;
                        break;
                }
        }
        if (device_info != NULL)
                XFreeDeviceList (device_info);

        return retval;
}
