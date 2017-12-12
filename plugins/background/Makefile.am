NULL =

noinst_PROGRAMS = 			\
	test-background			\
	$(NULL)

test_background_SOURCES = 		\
	test-background.c		\
	usd-background-manager.h	\
	usd-background-manager.c	\
	$(NULL)

test_background_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

test_background_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(MATE_DESKTOP_CFLAGS)		\
	$(AM_CFLAGS)

test_background_LDADD =		\
	$(top_builddir)/ukui-settings-daemon/libusd-profile.la	\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(MATE_DESKTOP_LIBS)		\
	$(X11_LIBS) \
	$(NULL)

plugin_LTLIBRARIES = \
	libbackground.la		\
	$(NULL)

libbackground_la_SOURCES = 		\
	usd-background-plugin.h		\
	usd-background-plugin.c		\
	usd-background-manager.h	\
	usd-background-manager.c	\
	$(NULL)

libbackground_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-I$(top_srcdir)/plugins/background/libbackground   \
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libbackground_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(MATE_DESKTOP_CFLAGS)		\
	$(AM_CFLAGS)

libbackground_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)		\
	$(NULL)

libbackground_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)		\
	$(MATE_DESKTOP_LIBS)		\
	$(NULL)

plugin_in_files = 		\
	background.ukui-settings-plugin.in	\
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
