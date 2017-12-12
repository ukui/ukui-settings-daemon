plugin_LTLIBRARIES = libmouse.la

libmouse_la_SOURCES = 		\
	usd-mouse-plugin.h	\
	usd-mouse-plugin.c	\
	usd-mouse-manager.h	\
	usd-mouse-manager.c

libmouse_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-I$(top_srcdir)/plugins/common/			\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DLIBEXECDIR=\""$(libexecdir)"\" \
	$(AM_CPPFLAGS)

libmouse_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

libmouse_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)

libmouse_la_LIBADD  =							\
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_PLUGIN_LIBS)						\
	$(X11_LIBS)							\
	$(XINPUT_LIBS)

plugin_in_files = mouse.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

libexec_PROGRAMS = usd-locate-pointer

usd_locate_pointer_SOURCES = 	\
	usd-locate-pointer.h	\
	usd-locate-pointer.c	\
	usd-timeline.h		\
	usd-timeline.c

usd_locate_pointer_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

usd_locate_pointer_LDADD  = 		\
	$(SETTINGS_PLUGIN_LIBS)		\
	$(X11_LIBS)			\
	-lm

EXTRA_DIST = $(plugin_in_files)
CLEANFILES = $(plugin_DATA)
DISTCLEANFILES = $(plugin_DATA)

@USD_INTLTOOL_PLUGIN_RULE@
