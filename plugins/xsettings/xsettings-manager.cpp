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
#include <QDebug>
#include "xsettings-manager.h"
#include "xsettings-const.h"
#include <X11/Xmd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <syslog.h>

static Time get_server_time (Display *display,
                             Window   window);
static XSettingsList *Settings;
XsettingsManager::XsettingsManager(Display                *display,
                                   int                     screen,
                                   XSettingsTerminateFunc  terminate,
                                   int                    *cb_data)
{
    XClientMessageEvent xev;

    this->display = display;
    this->screen = screen;

    char buffer[256];
    sprintf(buffer, "_XSETTINGS_S%d", this->screen);
    this->selection_atom = XInternAtom (display, buffer, False);
    this->xsettings_atom = XInternAtom (display, "_XSETTINGS_SETTINGS", False);
    this->manager_atom = XInternAtom (display, "MANAGER", False);

    this->terminate = terminate;
    this->cb_data = cb_data;
    this->settings = NULL;

    this->serial = 0;

    this->window = XCreateSimpleWindow (display,
                                        RootWindow (display, screen),
                                        0, 0, 10, 10, 0,
                                        WhitePixel (display, screen),
                                        WhitePixel (display, screen));

    XSelectInput (display, this->window, PropertyChangeMask);
    Time timestamp;

    timestamp = get_server_time (display, this->window);

    XSetSelectionOwner (display, this->selection_atom,
                        this->window, timestamp);

    /* Check to see if we managed to claim the selection. If not,
     * we treat it as if we got it then immediately lost it
     */
    if (XGetSelectionOwner (display, this->selection_atom) ==
            this->window)
    {
        xev.type = ClientMessage;
        xev.window = RootWindow (display, screen);
        xev.message_type = this->manager_atom;
        xev.format = 32;
        xev.data.l[0] = timestamp;
        xev.data.l[1] = this->selection_atom;
        xev.data.l[2] = this->window;
        xev.data.l[3] = 0;        /* manager specific data */
        xev.data.l[4] = 0;        /* manager specific data */

        XSendEvent (display, RootWindow (display, screen),
                    False, StructureNotifyMask, (XEvent *)&xev);
    }
    else
    {
        this->terminate ((int*)this->cb_data);
    }

}

XsettingsManager::~XsettingsManager()
{
    XDestroyWindow (display, window);
    xsettings_list_free (settings);
}

/**
 * get_server_time:
 * @display: display from which to get the time
 * @window: a #Window, used for communication with the server.
 *          The window must have PropertyChangeMask in its
 *          events mask or a hang will result.
 *
 * Routine to get the current X server time stamp.
 *
 * Return value: the time stamp.
 **/
typedef struct
{
    Window window;
    Atom timestamp_prop_atom;
} TimeStampInfo;

static Bool
timestamp_predicate (Display *display,
                     XEvent  *xevent,
                     XPointer arg)
{
    TimeStampInfo *info = (TimeStampInfo *)arg;

    if (xevent->type == PropertyNotify &&
        xevent->xproperty.window == info->window &&
        xevent->xproperty.atom == info->timestamp_prop_atom)
        return True;
    return False;
}

static Time get_server_time (Display *display,
                             Window   window)
{
    unsigned char c = 'a';
    XEvent xevent;
    TimeStampInfo info;

    info.timestamp_prop_atom = XInternAtom  (display, "_TIMESTAMP_PROP", False);
    info.window = window;

    XChangeProperty (display, window,
                     info.timestamp_prop_atom, info.timestamp_prop_atom,
                     8, PropModeReplace, &c, 1);

    XIfEvent (display, &xevent,
              timestamp_predicate, (XPointer)&info);

    return xevent.xproperty.time;
}

Window XsettingsManager::get_window()
{
    return window;
}

Bool   XsettingsManager::process_event (XEvent           *xev)
{
    if (xev->xany.window == this->window &&
            xev->xany.type == SelectionClear &&
            xev->xselectionclear.selection == this->selection_atom)
    {
        this->terminate ((int *)this->cb_data);
        return True;
    }

    return False;
}

XSettingsResult XsettingsManager::delete_setting (const char       *name)
{
    return xsettings_list_delete (&Settings, name);
}

XSettingsResult XsettingsManager::set_setting    (XSettingsSetting *setting)
{
    XSettingsSetting *old_setting = xsettings_list_lookup (Settings, setting->name);
    XSettingsSetting *new_setting;
    XSettingsResult result;
    if (old_setting)
    {
        if (xsettings_setting_equal (old_setting, setting))
            return XSETTINGS_SUCCESS;

        xsettings_list_delete (&Settings, setting->name);
    }
    new_setting = xsettings_setting_copy (setting);
    if (!new_setting)
        return XSETTINGS_NO_MEM;

    new_setting->last_change_serial = this->serial;

    result = xsettings_list_insert (&Settings, new_setting);

    if (result != XSETTINGS_SUCCESS)
        xsettings_setting_free (new_setting);
    return result;
}

XSettingsResult XsettingsManager::set_int        (const char       *name,
                                                  int               value)
{
    XSettingsSetting setting;

    setting.name = (char *)name;
    setting.type = XSETTINGS_TYPE_INT;
    setting.data.v_int = value;

    return set_setting (&setting);
}

XSettingsResult XsettingsManager::set_string     (const char       *name,
                                                  const char       *value)
{
    XSettingsSetting setting;

    setting.name = (char *)name;
    setting.type = XSETTINGS_TYPE_STRING;
    setting.data.v_string = (char *)value;

    return set_setting (&setting);
}

XSettingsResult XsettingsManager::set_color      (const char       *name,
                                                  XSettingsColor   *value)
{
    XSettingsSetting setting;

    setting.name = (char *)name;
    setting.type = XSETTINGS_TYPE_COLOR;
    setting.data.v_color = *value;

    return set_setting (&setting);

}
static size_t setting_length (XSettingsSetting *setting)
{
    size_t length = 8;    /* type + pad + name-len + last-change-serial */
    length += XSETTINGS_PAD (strlen (setting->name), 4);

    switch (setting->type)
    {
    case XSETTINGS_TYPE_INT:
        length += 4;
        break;
    case XSETTINGS_TYPE_STRING:
        length += 4 + XSETTINGS_PAD (strlen (setting->data.v_string), 4);
        break;
    case XSETTINGS_TYPE_COLOR:
        length += 8;
        break;
    }

    return length;
}

Bool xsettings_manager_check_running (Display *display,
                                      int      screen)
{
    char buffer[256];
    Atom selection_atom;

    sprintf(buffer, "_XSETTINGS_S%d", screen);
    selection_atom = XInternAtom (display, buffer, False);

    gdk_x11_display_error_trap_push (gdk_display_get_default());
    if (XGetSelectionOwner (display, selection_atom)){
        gdk_x11_display_error_trap_pop_ignored (gdk_display_get_default());

        return True;
    }
    else
        return False;
}

void XsettingsManager::setting_store (XSettingsSetting *setting,
                                      XSettingsBuffer *buffer)
{
    size_t string_len;
    size_t length;

    *(buffer->pos++) = setting->type;
    *(buffer->pos++) = 0;

    string_len = strlen (setting->name);
    *(CARD16 *)(buffer->pos) = string_len;
    buffer->pos += 2;

    length = XSETTINGS_PAD (string_len, 4);
    memcpy (buffer->pos, setting->name, string_len);
    length -= string_len;
    buffer->pos += string_len;

    while (length > 0)
    {
        *(buffer->pos++) = 0;
        length--;
    }

    *(CARD32 *)(buffer->pos) = setting->last_change_serial;
    buffer->pos += 4;

    switch (setting->type)
    {
    case XSETTINGS_TYPE_INT:
        *(CARD32 *)(buffer->pos) = setting->data.v_int;
        buffer->pos += 4;
        break;
    case XSETTINGS_TYPE_STRING:
        string_len = strlen (setting->data.v_string);
        *(CARD32 *)(buffer->pos) = string_len;
        buffer->pos += 4;

        length = XSETTINGS_PAD (string_len, 4);
        memcpy (buffer->pos, setting->data.v_string, string_len);
        length -= string_len;
        buffer->pos += string_len;

        while (length > 0)
        {
            *(buffer->pos++) = 0;
            length--;
        }
        break;
    case XSETTINGS_TYPE_COLOR:
        *(CARD16 *)(buffer->pos) = setting->data.v_color.red;
        *(CARD16 *)(buffer->pos + 2) = setting->data.v_color.green;
        *(CARD16 *)(buffer->pos + 4) = setting->data.v_color.blue;
        *(CARD16 *)(buffer->pos + 6) = setting->data.v_color.alpha;
        buffer->pos += 8;
        break;
    }
}

XSettingsResult XsettingsManager::notify()
{
    XSettingsBuffer buffer;
    XSettingsList *iter;
    int n_settings = 0;
    buffer.len = 12;              /* byte-order + pad + SERIAL + N_SETTINGS */
    iter = Settings;
    while (iter)
    {
        buffer.len += setting_length (iter->setting);
        n_settings++;
        iter = iter->next;
    }
    buffer.data = buffer.pos = new unsigned char[buffer.len];
    if (!buffer.data)
        return XSETTINGS_NO_MEM;

    *buffer.pos = xsettings_byte_order ();

    buffer.pos += 4;
    *(CARD32 *)buffer.pos = this->serial++;
    buffer.pos += 4;
    *(CARD32 *)buffer.pos = n_settings;
    buffer.pos += 4;

    iter = Settings;
    while (iter)
    {
        setting_store (iter->setting, &buffer);
        iter = iter->next;
    }

    XChangeProperty (this->display, this->window,
                     this->xsettings_atom, this->xsettings_atom,
                     8, PropModeReplace, buffer.data, buffer.len);
    free (buffer.data);

    return XSETTINGS_SUCCESS;
}
