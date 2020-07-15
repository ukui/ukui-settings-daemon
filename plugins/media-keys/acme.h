#ifndef __ACME_H__
#define __ACME_H__

#include "usd-keygrab.h"

#define BINDING_SCHEMA "org.ukui.SettingsDaemon.plugins.media-keys"

enum {
        TOUCHPAD_KEY,
        MUTE_KEY,
        VOLUME_DOWN_KEY,
        VOLUME_UP_KEY,
        POWER_KEY,
        EJECT_KEY,
        HOME_KEY,
        MEDIA_KEY,
        CALCULATOR_KEY,
        SEARCH_KEY,
        EMAIL_KEY,
        SCREENSAVER_KEY,
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
        FILE_MANAGER_KEY,
        ON_SCREEN_KEYBOARD_KEY,
        LOGOUT_KEY,
        TERMINAL_KEY,
        SCREENSHOT_KEY,
        WINDOW_SCREENSHOT_KEY,
        AREA_SCREENSHOT_KEY,
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
        { POWER_KEY, "power", NULL, NULL },
        { EJECT_KEY, "eject", NULL, NULL },
        { HOME_KEY, "home", NULL, NULL },
        { MEDIA_KEY, "media", NULL, NULL },
        { CALCULATOR_KEY, "calculator", NULL, NULL },
        { SEARCH_KEY, "search", NULL, NULL },
        { EMAIL_KEY, "email", NULL, NULL },
        { SCREENSAVER_KEY, "screensaver", NULL, NULL },
        { SETTINGS_KEY, "ukui-control-center", NULL, NULL},
        { FILE_MANAGER_KEY, "peony-qt", NULL, NULL},
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
        { TERMINAL_KEY, "terminal", NULL, NULL },
        { SCREENSHOT_KEY, "screenshot", NULL, NULL },
        { WINDOW_SCREENSHOT_KEY, "window-screenshot", NULL, NULL },
        { AREA_SCREENSHOT_KEY, "area-screenshot", NULL, NULL },
};

#endif /* __ACME_H__ */
