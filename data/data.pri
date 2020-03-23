OTHER_FILES += \
    $$PWD/ukui-settings-daemon.desktop\
    $$PWD/org.ukui.SettingsDaemon.service\
    \
    $$PWD/sound.ukui-settings-plugin \
    $$PWD/mpris.ukui-settings-plugin_bak \
    $$PWD/background.ukui-settings-plugin \
    $$PWD/clipboard.ukui-settings-plugin_bak \
    $$PWD/typing-break.ukui-settings-plugin\
    $$PWD/a11y-settings.ukui-settings-plugin \
    \
    $$PWD/org.ukui.SettingsDaemon.plugins.mpris.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.sound.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.clipboard.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.background.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.typing-break.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.a11y-settings.gschema.xml

# desktop ok
desktop.path = /usr/share/gnome/autostart/
desktop.files = $$PWD/ukui-settings-daemon.desktop

# daemon ok
ukui_daemon.path = /usr/bin/
ukui_daemon.files = $$PWD/../daemon/ukui-settings-daemon

# plugins
plugin_lib.path = /usr/local/lib/ukui-settings-daemon/
plugin_lib.files = $$PWD/../library/*

plugin_info.path = /usr/local/lib/ukui-settings-daemon/
plugin_info.files = $$PWD/*.ukui-settings-plugin

plugin_schema.path = /usr/share/glib-2.0/schemas/
plugin_schema.files = $$PWD/org.ukui.SettingsDaemon.plugins.*.gschema.xml

# dbus
ukui_daemon_dbus.path = /usr/share/dbus-1/services/
ukui_daemon_dbus.files = $$PWD/org.ukui.SettingsDaemon.service

INSTALLS += desktop ukui_daemon plugin_lib plugin_info plugin_schema ukui_daemon_dbus

