plugin_LTLIBRARIES = \
	libmpris.la

libmpris_la_SOURCES = 		\
	usd-mpris-manager.c	\
	usd-mpris-manager.h	\
	usd-mpris-plugin.c	\
	usd-mpris-plugin.h

libmpris_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libmpris_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libmpris_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)

libmpris_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)

plugin_in_files = 		\
	mpris.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = 			\
	$(plugin_in_files)

CLEANFILES = 			\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@USD_INTLTOOL_PLUGIN_RULE@

