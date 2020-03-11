#documentation.path = /usr/local/program/doc
#  documentation.files = docs/*

# desktop ok
desktop.path = /usr/share/gnome/autostart/
desktop.files = $$PWD/ukui-settings-daemon.desktop

# daemon ok
ukui_daemon.path = /usr/bin/
ukui_daemon.files = $$PWD/../daemon/ukui-settings-daemon

# plugins
plugin_lib.path = /usr/local/lib/ukui-settings-daemon/
plugin_lib.files = $$PWD/../library/*

plugin_info.path = /usr/local/lib/ukui-settings-daemon
plugin_info.files = $$PWD/*.ukui-settings-plugin

plugin_schema.path = /usr/share/glib-2.0/schemas/
plugin_schema.files = $$PWD/org.ukui.SettingsDaemon.plugins.*.gschema.xml

INSTALLS += desktop ukui_daemon plugin_lib plugin_info plugin_schema
