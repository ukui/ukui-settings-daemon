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

#define LEFT_SHIFT 0x32
#define LEFT_CTRL   0x25
#define LEFT_ALT    0x40
#define RIGHT_SHIFT 0x3E
#define RIGHT_ALT 0x6C
#define RIGHT_CTRL 0x69
#define MATE_KEY 0x85

#define xEventHandleHadRelase(action)     bool action##HadRelease = false

#define xEventHandleRelease(action)       action##HadRelease = false;

#define xEventHandle(action,xEvent)       if (false == action##HadRelease) {          \
                                                        doAction(action);             \
                                                        action##HadRelease = true;             \
                                                    }

#define STATE_OFF     0
#define STATE_ON      1
#define STATE_TOGGLE  2

/*Xorg shutkey nameEnd*/

#endif // USD_GLOBAL_DEFINE_H
