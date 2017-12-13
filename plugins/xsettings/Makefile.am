NULL =

plugin_LTLIBRARIES = \
	libxsettings.la		\
	$(NULL)

libxsettings_la_SOURCES = 	\
	usd-xsettings-plugin.h	\
	usd-xsettings-plugin.c	\
	usd-xsettings-manager.h	\
	usd-xsettings-manager.c	\
	xsettings-common.h	\
	xsettings-common.c	\
	xsettings-manager.h	\
	xsettings-manager.c	\
	fontconfig-monitor.h	\
	fontconfig-monitor.c	\
	$(NULL)

libxsettings_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libxsettings_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(FONTCONFIG_CFLAGS)		\
	$(AM_CFLAGS)

libxsettings_la_LDFLAGS = 	\
	$(USD_PLUGIN_LDFLAGS)	\
	$(NULL)

libxsettings_la_LIBADD  = 	\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(FONTCONFIG_LIBS)	\
	$(NULL)

plugin_in_files = 		\
	xsettings.ukui-settings-plugin.in	\
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
