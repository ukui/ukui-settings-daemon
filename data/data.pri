#-------------------------------------------------
#
# Project created by QtCreator 2020-03-17T09:30:00
#
#-------------------------------------------------
OTHER_FILES += \
    $$PWD/ukui-settings-daemon.desktop      \
    $$PWD/org.ukui.SettingsDaemon.service   \
    $$PWD/org.ukui.font-rendering.gschema.xml.in \
    $$PWD/org.ukui.SettingsDaemon.plugins.a11y-settings.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.a11y-keyboard.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.auto-brightness.gschema.xml    \
 #   $$PWD/org.ukui.SettingsDaemon.plugins.background.gschema.xml    \
    $$PWD/org.ukui.SettingsDaemon.plugins.clipboard.gschema.xml     \
    $$PWD/org.ukui.SettingsDaemon.plugins.color.gschema.xml         \
    $$PWD/org.ukui.SettingsDaemon.plugins.housekeeping.gschema.xml  \
    $$PWD/org.ukui.SettingsDaemon.plugins.keybindings.gschema.xml   \
    $$PWD/org.ukui.SettingsDaemon.plugins.keyboard.gschema.xml    \
    $$PWD/org.ukui.SettingsDaemon.plugins.mpris.gschema.xml       \
    $$PWD/org.ukui.SettingsDaemon.plugins.mouse.gschema.xml       \
    $$PWD/org.ukui.SettingsDaemon.plugins.media-keys.gschema.xml  \
    $$PWD/org.ukui.SettingsDaemon.plugins.sound.gschema.xml       \
    $$PWD/org.ukui.SettingsDaemon.plugins.sharing.gschema.xml       \
    $$PWD/org.ukui.SettingsDaemon.plugins.tablet-mode.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.xinput.gschema.xml   \
    $$PWD/org.ukui.SettingsDaemon.plugins.xrandr.gschema.xml      \
    $$PWD/org.ukui.SettingsDaemon.plugins.xrdb.gschema.xml        \
    $$PWD/org.ukui.SettingsDaemon.plugins.xsettings.gschema.xml   \
    $$PWD/org.ukui.peripherals-keyboard.gschema.xml         \
    $$PWD/org.ukui.peripherals-mouse.gschema.xml            \
    $$PWD/org.ukui.peripherals-touchpad.gschema.xml         \
    $$PWD/org.ukui.peripherals-touchscreen.gschema.xml      \
    $$PWD/zh_CN.po  \
    $$PWD/power-ignore.conf \
    $$PWD/61-keyboard.hwdb  \

# desktop ok
desktop.path = /etc/xdg/autostart/
desktop.files = $$PWD/ukui-settings-daemon.desktop

plugin_info.path = $$[QT_INSTALL_LIBS]/ukui-settings-daemon
plugin_info.files = $$PWD/*.ukui-settings-plugin

plugin_schema.path = /usr/share/glib-2.0/schemas/
plugin_schema.files = $$PWD/org.ukui.*.gschema.xml

# dbus
ukui_daemon_dbus.path = /usr/share/dbus-1/services/
ukui_daemon_dbus.files = $$PWD/org.ukui.SettingsDaemon.service

zhCN.commands = $$system(msgfmt -o ukui-settings-daemon.mo $$PWD/zh_CN.po)
QMAKE_EXTRA_TARGETS += zhCN

zh_CN.path  = /usr/share/locale/zh_CN/LC_MESSAGES/
zh_CN.files = $$PWD/ukui-settings-daemon.mo

power.path  = /etc/systemd/logind.conf.d/
power.files += $$PWD/power-ignore.conf

#touchpad hotkey
touchpad_udev.path = /etc/udev/hwdb.d/
touchpad_udev.files = $$PWD/61-keyboard.hwdb

INSTALLS += desktop plugin_info plugin_schema ukui_daemon_dbus zh_CN power touchpad_udev

DISTFILES += \
    $$PWD/auto-brightness.ukui-settings-plugin  \
    $$PWD/a11y-settings.ukui-settings-plugin \
    $$PWD/a11y-keyboard.ukui-settings-plugin \
    $$PWD/background.ukui-settings-plugin    \
    $$PWD/clipboard.ukui-settings-plugin     \
    $$PWD/color.ukui-settings-plugin         \
    $$PWD/housekeeping.ukui-settings-plugin  \
    $$PWD/media-keys.ukui-settings-plugin    \
    $$PWD/mouse.ukui-settings-plugin         \
    $$PWD/mpris.ukui-settings-plugin         \
    $$PWD/keyboard.ukui-settings-plugin      \
    $$PWD/keybindings.ukui-settings-plugin   \
    $$PWD/sound.ukui-settings-plugin         \
    $$PWD/sharing.ukui-settings-plugin       \
    $$PWD/tablet-mode.ukui-settings-plugin   \
    $$PWD/xrdb.ukui-settings-plugin          \
    $$PWD/xrandr.ukui-settings-plugin        \
    $$PWD/xsettings.ukui-settings-plugin    \
    $$PWD/xinput.ukui-settings-plugin
