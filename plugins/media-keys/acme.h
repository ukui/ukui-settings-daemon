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

#ifndef __ACME_H__
#define __ACME_H__

#include "ukui-keygrab.h"

#define BINDING_SCHEMA "org.ukui.SettingsDaemon.plugins.media-keys"

enum {
    TOUCHPAD_KEY,
    MUTE_KEY,
    VOLUME_DOWN_KEY,
    VOLUME_UP_KEY,
    MIC_MUTE_KEY,
    BRIGHT_UP_KEY,
    BRIGHT_DOWN_KEY,
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
    WLAN_KEY,
    WEBCAM_KEY,
    HANDLED_KEYS,
    UKUI_SIDEBAR,
    UKUI_EYECARE_CENTER,
    TOUCHPAD_ON_KEY,
    TOUCHPAD_OFF_KEY,
    RFKILL_KEY,
    BLUETOOTH_KEY,
};

static struct {
        int key_type;
        const char *settings_key;
        const char *hard_coded;
        Key *key;
} keys[HANDLED_KEYS] = {
};

#endif /* __ACME_H__ */
