NULL =

usd_gschemas_in = \
	org.ukui.applications-at.gschema.xml.in \
	org.ukui.font-rendering.gschema.xml.in \
	org.ukui.peripherals-smartcard.gschema.xml.in \
	org.ukui.peripherals-touchpad.gschema.xml.in \
	org.ukui.peripherals-touchscreen.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.a11y-keyboard.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.a11y-settings.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.background.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.clipboard.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.datetime.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.housekeeping.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.keybindings.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.keyboard.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.media-keys.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.mpris.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.mouse.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.smartcard.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.sound.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.typing-break.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.xrandr.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.xrdb.gschema.xml.in \
	org.ukui.SettingsDaemon.plugins.xsettings.gschema.xml.in \
	$(NULL)

@INTLTOOL_XML_NOMERGE_RULE@

gsettings_SCHEMAS = $(usd_gschemas_in:.xml.in=.xml)

@GSETTINGS_RULES@

@INTLTOOL_DESKTOP_RULE@
desktopdir = $(sysconfdir)/xdg/autostart
desktop_in_files = ukui-settings-daemon.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

servicedir = $(datadir)/dbus-1/services
service_in_files = org.ukui.SettingsDaemon.service.in
service_DATA = $(service_in_files:.service.in=.service)

org.ukui.SettingsDaemon.service: org.ukui.SettingsDaemon.service.in Makefile
	$(AM_V_GEN) sed -e "s|\@libexecdir\@|$(libexecdir)|" $< > $@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ukui-settings-daemon.pc

man_MANS = \
	ukui-settings-daemon.1 \
	usd-datetime-mechanism.1 \
	usd-locate-pointer.1

@INTLTOOL_XML_NOMERGE_RULE@

#disabled until we support it
xmldir = @UKUI_KEYBINDINGS_KEYSDIR@
xml_in_files =
xml_DATA = $(xml_in_files:.xml.in=.xml)

EXTRA_DIST = 				\
	$(usd_gschemas_in)		\
	$(service_in_files)		\
	$(desktop_in_files)		\
	$(xml_in_files)			\
	ukui-settings-daemon.pc.in	\
	$(man_MANS)				\
	$(NULL)

DISTCLEANFILES = 			\
	$(gsettings_SCHEMAS)			\
	$(service_DATA)			\
	$(desktop_DATA)			\
	$(xml_DATA)			\
	$(NULL)

MAINTAINERCLEANFILES =			\
	*~				\
	Makefile.in
