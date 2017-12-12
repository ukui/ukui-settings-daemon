NULL =

plugin_LTLIBRARIES = \
	libtyping-break.la		\
	$(NULL)

libtyping_break_la_SOURCES = 		\
	usd-typing-break-plugin.h	\
	usd-typing-break-plugin.c	\
	usd-typing-break-manager.h	\
	usd-typing-break-manager.c	\
	$(NULL)

libtyping_break_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libtyping_break_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libtyping_break_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)	\
	$(NULL)

libtyping_break_la_LIBADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(NULL)

plugin_in_files = 		\
	typing-break.ukui-settings-plugin.in	\
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
