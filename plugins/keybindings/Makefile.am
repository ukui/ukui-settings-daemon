NULL =

plugin_LTLIBRARIES = \
	libkeybindings.la		\
	$(NULL)

libkeybindings_la_SOURCES = 		\
	dconf-util.h				\
	dconf-util.c				\
	usd-keybindings-plugin.h	\
	usd-keybindings-plugin.c	\
	usd-keybindings-manager.h	\
	usd-keybindings-manager.c	\
	$(NULL)

libkeybindings_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-I$(top_srcdir)/plugins/common			\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libkeybindings_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(DCONF_CFLAGS)	\
	$(AM_CFLAGS)

libkeybindings_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)	\
	$(NULL)

libkeybindings_la_LIBADD  = 		\
	$(top_builddir)/plugins/common/libcommon.la	\
	$(SETTINGS_PLUGIN_LIBS)		\
	$(DCONF_LIBS)	\
	$(NULL)

plugin_in_files = 		\
	keybindings.ukui-settings-plugin.in	\
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
