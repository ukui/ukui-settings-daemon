#ifndef USD_GLOBAL_DEFINE_H
#define USD_GLOBAL_DEFINE_H

#define USD_BTN_PRESS   2
#define USD_BTN_RELEASE 3


/*com.kylin.statusmanager.interface*/
#define DBUS_STATUSMANAGER_NAME "com.kylin.statusmanager.interface"
#define DBUS_STATUSMANAGER_PATH "/"
#define DBUS_STATUSMANAGER_INTERFACE "com.kylin.statusmanager.interface"
#define DBUS_STATUSMANAGER_GET_MODE  "get_current_tabletmode"
#define DBUS_STATUSMANAGER_GET_ROTATION "get_current_rotation"
/*****/

/*org.ukui.SettingsDaemon.xrandr*/
#define DBUS_XRANDR_NAME "org.ukui.SettingsDaemon"
#define DBUS_XRANDR_PATH "/org/ukui/SettingsDaemon/xrandr"
#define DBUS_XRANDR_INTERFACE "org.ukui.SettingsDaemon.xrandr"
#define DBUS_XRANDR_GET_MODE  "getScreenMode"
#define DBUS_XRANDR_SET_MODE "setScreenMode"
#define DBUS_XRANDR_GET_SCREEN_PARAM "getScreensParam"
/*****/


/*Xorg shutkey name*/

#define xEventHandle(action,xEvent)       static bool action##hadRelease = false; \
                                                    if (xEvent->u.u.type == USD_BTN_RELEASE) {   \
                                                        action##hadRelease = false;         \
                                                    }                                       \
                                                    else if (false == action##hadRelease) {          \
                                                        doAction(action);             \
                                                        action##hadRelease = true;             \
                                                    }

#define X_SHUTKEY_XF86AudioMute             "XF86AudioMute"
#define X_SHUTKEY_XF86AudioRaiseVolume      "XF86AudioRaiseVolume"
#define X_SHUTKEY_XF86AudioLowerVolume      "XF86AudioLowerVolume"
#define X_SHUTKEY_XF86MonBrightnessDown     "XF86MonBrightnessDown"
#define X_SHUTKEY_XF86MonBrightnessUp       "XF86MonBrightnessUp"
#define X_SHUTKEY_XF86RFKill                "XF86RFKill"
#define X_SHUTKEY_PRINT                     "Print"     //截图
#define X_SHUTKEY_XF86TouchpadToggle        "XF86TouchpadToggle"     //截图
/*Xorg shutkey nameEnd*/

#endif // USD_GLOBAL_DEFINE_H
