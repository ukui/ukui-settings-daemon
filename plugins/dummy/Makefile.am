plugin_LTLIBRARIES = \
	libdummy.la

libdummy_la_SOURCES = 		\
	usd-dummy-manager.c	\
	usd-dummy-manager.h	\
	usd-dummy-plugin.c	\
	usd-dummy-plugin.h

libdummy_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libdummy_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libdummy_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)

libdummy_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)

plugin_in_files = 		\
	dummy.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = 			\
	$(plugin_in_files)

CLEANFILES = 			\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@USD_INTLTOOL_PLUGIN_RULE@

# override to _not_ install the test plugin
# do not copy into your plugin
install-pluginDATA:
install-pluginLTLIBRARIES:
