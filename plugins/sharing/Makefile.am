plugin_LTLIBRARIES = \
	libsharing.la

libsharing_la_SOURCES = 		\
	usd-sharing-manager.c	\
	usd-sharing-manager.h	\
	usd-sharing-plugin.c	\
	usd-sharing-plugin.h

libsharing_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libsharing_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libsharing_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)

libsharing_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)

plugin_in_files = 		\
	sharing.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = 			\
	$(plugin_in_files)

CLEANFILES = 			\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@USD_INTLTOOL_PLUGIN_RULE@

