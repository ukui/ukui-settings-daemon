NULL =

SUBDIRS =		\
	data		\
	$(NULL)

plugin_LTLIBRARIES = \
	libxrdb.la		\
	$(NULL)

libxrdb_la_SOURCES = 		\
	usd-xrdb-plugin.h	\
	usd-xrdb-plugin.c	\
	usd-xrdb-manager.h	\
	usd-xrdb-manager.c	\
	$(NULL)

libxrdb_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DSYSCONFDIR=\""$(sysconfdir)"\"	\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libxrdb_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libxrdb_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)	\
	$(NULL)

libxrdb_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(NULL)

plugin_in_files = 		\
	xrdb.ukui-settings-plugin.in	\
	$(NULL)

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = 			\
	$(plugin_in_files)	\
	$(NULL)

CLEANFILES = 			\
	$(plugin_DATA)		\
	$(NULL)

DISTCLEANFILES =		\
	$(plugin_DATA)		\
	$(NULL)

@USD_INTLTOOL_PLUGIN_RULE@
