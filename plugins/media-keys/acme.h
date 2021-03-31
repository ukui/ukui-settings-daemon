/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2001 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2017 Tianjin KYLIN Information Technology Co., Ltd.
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

#ifndef __ACME_H__
#define __ACME_H__

#include "usd-keygrab.h"

#define BINDING_SCHEMA "org.ukui.SettingsDaemon.plugins.media-keys"

enum {
        TOUCHPAD_KEY,
        MUTE_KEY,
        VOLUME_DOWN_KEY,
        VOLUME_UP_KEY,
        MIC_MUTE_KEY,
        POWER_KEY,
        EJECT_KEY,
        HOME_KEY,
        MEDIA_KEY,
        CALCULATOR_KEY,
        SEARCH_KEY,
        EMAIL_KEY,
        SCREENSAVER_KEY,
        SCREENSAVER_KEY_2,
        HELP_KEY,
        WWW_KEY,
        PLAY_KEY,
        PAUSE_KEY,
        STOP_KEY,
        PREVIOUS_KEY,
        NEXT_KEY,
        REWIND_KEY,
        FORWARD_KEY,
        REPEAT_KEY,
        RANDOM_KEY,
        MAGNIFIER_KEY,
        SCREENREADER_KEY,
        SETTINGS_KEY,
        SETTINGS_KEY_2,
        FILE_MANAGER_KEY,
        FILE_MANAGER_KEY_2,
        ON_SCREEN_KEYBOARD_KEY,
        LOGOUT_KEY,
        LOGOUT_KEY1,
        LOGOUT_KEY2,
        TERMINAL_KEY,
        TERMINAL_KEY_2,
        SCREENSHOT_KEY,
        WINDOW_SCREENSHOT_KEY,
        AREA_SCREENSHOT_KEY,
        WINDOWSWITCH_KEY,
        WINDOWSWITCH_KEY_2,
        SYSTEM_MONITOR_KEY,
        CONNECTION_EDITOR_KEY,
        GLOBAL_SEARCH_KEY,
        KDS_KEY,
        KDS_KEY2,
        HANDLED_KEYS,
};

static struct {
        int key_type;
        const char *settings_key;
        const char *hard_coded;
        Key *key;
} keys[HANDLED_KEYS] = {
        { TOUCHPAD_KEY, "touchpad", NULL, NULL },
        { MUTE_KEY, "volume-mute", NULL, NULL },
        { VOLUME_DOWN_KEY, "volume-down", NULL, NULL },
        { VOLUME_UP_KEY, "volume-up", NULL, NULL },
        { MIC_MUTE_KEY, "mic-mute", NULL, NULL},
        { POWER_KEY, "power", NULL, NULL },
        { EJECT_KEY, "eject", NULL, NULL },
        { HOME_KEY, "home", NULL, NULL },
        { MEDIA_KEY, "media", NULL, NULL },
        { CALCULATOR_KEY, "calculator", NULL, NULL },
        { SEARCH_KEY, "search", NULL, NULL },
        { EMAIL_KEY, "email", NULL, NULL },
        { SCREENSAVER_KEY, "screensaver", NULL, NULL },
        { SCREENSAVER_KEY_2, "screensaver2", NULL, NULL },
        { SETTINGS_KEY, "ukui-control-center", NULL, NULL},
        { SETTINGS_KEY_2, "ukui-control-center2", NULL, NULL},
        { FILE_MANAGER_KEY, "peony-qt", NULL, NULL},
        { FILE_MANAGER_KEY_2, "peony-qt2", NULL, NULL},
        { HELP_KEY, "help", NULL, NULL },
        { WWW_KEY, "www", NULL, NULL },
        { PLAY_KEY, "play", NULL, NULL },
        { PAUSE_KEY, "pause", NULL, NULL },
        { STOP_KEY, "stop", NULL, NULL },
        { PREVIOUS_KEY, "previous", NULL, NULL },
        { NEXT_KEY, "next", NULL, NULL },
        /* Those are not configurable in the UI */
        { REWIND_KEY, NULL, "XF86AudioRewind", NULL },
        { FORWARD_KEY, NULL, "XF86AudioForward", NULL },
        { REPEAT_KEY, NULL, "XF86AudioRepeat", NULL },
        { RANDOM_KEY, NULL, "XF86AudioRandomPlay", NULL },
        { MAGNIFIER_KEY, "magnifier", NULL, NULL },
        { SCREENREADER_KEY, "screenreader", NULL, NULL },
        { ON_SCREEN_KEYBOARD_KEY, "on-screen-keyboard", NULL, NULL },
        { LOGOUT_KEY, "logout", NULL, NULL },
        { LOGOUT_KEY1, "logout1", NULL, NULL },
        { LOGOUT_KEY2, "logout2", NULL, NULL },
        { TERMINAL_KEY, "terminal", NULL, NULL },
        { TERMINAL_KEY_2, "terminal2", NULL, NULL },
        //{ SCREENSHOT_KEY, "screenshot", NULL, NULL },
        { WINDOW_SCREENSHOT_KEY, "window-screenshot", NULL, NULL },
        //{ AREA_SCREENSHOT_KEY, "area-screenshot", NULL, NULL },
        { WINDOWSWITCH_KEY, "ukui-window-switch", NULL, NULL},
        { WINDOWSWITCH_KEY_2, "ukui-window-switch2", NULL, NULL},
        //{ SYSTEM_MONITOR_KEY, "ukui-system-monitor", NULL, NULL },
        { CONNECTION_EDITOR_KEY, "nm-connection-editor", NULL, NULL },
        { GLOBAL_SEARCH_KEY, "ukui-search", NULL, NULL },
        { KDS_KEY, "kylin-display-switch", NULL, NULL },
        { KDS_KEY2, "kylin-display-switch2", NULL, NULL },
};

#endif /* __ACME_H__ */
