OTHER_FILES += \
    $$PWD/ukui-settings-daemon.desktop\
    $$PWD/org.ukui.SettingsDaemon.service\
    \
    $$PWD/background.ukui-settings-plugin \
    \
    $$PWD/org.ukui.SettingsDaemon.plugins.mpris.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.sound.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.clipboard.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.background.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.typing-break.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.a11y-settings.gschema.xml \
    $$PWD/org.ukui.SettingsDaemon.plugins.keyboard.gschema.xml \
    $$PWD/org.ukui.peripherals-keyboard.gschema.xml	\
    $$PWD/org.ukui.peripherals-mouse.gschema.xml    \
    $$PWD/org.ukui.SettingsDaemon.plugins.mouse.gschema.xml \
    $$PWD/org.ukui.peripherals-smartcard.gschema.xml     \
    $$PWD/org.ukui.SettingsDaemon.plugins.smartcard.gschema.xml

# desktop ok
desktop.path = /usr/share/gnome/autostart/
desktop.files = $$PWD/ukui-settings-daemon.desktop

plugin_info.path = /usr/local/lib/ukui-settings-daemon/
plugin_info.files = $$PWD/*.ukui-settings-plugin

plugin_schema.path = /usr/share/glib-2.0/schemas/
plugin_schema.files = $$PWD/org.ukui.*.gschema.xml

# dbus
ukui_daemon_dbus.path = /usr/share/dbus-1/services/
ukui_daemon_dbus.files = $$PWD/org.ukui.SettingsDaemon.service

INSTALLS += desktop plugin_info plugin_schema ukui_daemon_dbus

DISTFILES += \
    $$PWD/a11y-settings.ukui-settings-plugin \
    $$PWD/clipboard.ukui-settings-plugin_bak \
    $$PWD/media-keys.ukui-settings-plugin_bak \
    $$PWD/mouse.ukui-settings-plugin  \
    $$PWD/mpris.ukui-settings-plugin \
    $$PWD/sound.ukui-settings-plugin_bak \
    $$PWD/typing-break.ukui-settings-plugin \
    $$PWD/xrandr.ukui-settings-plugin_bak \
    $$PWD/xrdb.ukui-settings-plugin_bak \
    $$PWD/keyboard.ukui-settings-plugin \
    $$PWD/smartcard.ukui-settings-plugin

