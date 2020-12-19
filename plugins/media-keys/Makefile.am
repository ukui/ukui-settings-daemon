icondir = $(datadir)/icons/ukui
context = actions

NULL =

plugin_LTLIBRARIES = libmedia-keys.la

BUILT_SOURCES = 			\
	usd-media-keys-manager-glue.h	\
	usd-marshal.h			\
	usd-marshal.c			\
	$(NULL)

usd-media-keys-manager-glue.h: usd-media-keys-manager.xml Makefile
	$(AM_V_GEN) dbus-binding-tool --prefix=usd_media_keys_manager --mode=glib-server $< > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ ) \
	&& rm -f xgen-$(@F)

usd-marshal.c: usd-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=usd_marshal $< --header --body --internal > $@

usd-marshal.h: usd-marshal.list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) --prefix=usd_marshal $< --header --internal > $@

libmedia_keys_la_SOURCES = 		\
	usd-media-keys-plugin.h		\
	usd-media-keys-plugin.c		\
	usd-media-keys-manager.h	\
	usd-media-keys-manager.c	\
	usd-media-keys-window.h		\
	usd-media-keys-window.c		\
	acme.h				\
	$(BUILT_SOURCES)		\
	$(NULL)

libmedia_keys_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon			\
	-I$(top_srcdir)/plugins/common				\
	-DPIXMAPDIR=\""$(pkgdatadir)"\"				\
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"			\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

libmedia_keys_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
    $(LIBMATEMIXER_CFLAGS)   \
    $(LIBCANBERRA_CFLAGS)   \
	$(AM_CFLAGS)

libmedia_keys_la_LDFLAGS = 		\
	$(USD_PLUGIN_LDFLAGS)

libmedia_keys_la_LIBADD  = 		\
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_PLUGIN_LIBS)						\
    $(LIBMATEMIXER_LIBS)                     \
    $(LIBCANBERRA_LIBS)         \
	$(XF86MISC_LIBS)						\
	-lm

plugin_in_files = 		\
	media-keys.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

noinst_PROGRAMS =				\
	test-media-keys				\
	test-media-window			\
	$(NULL)

test_media_window_SOURCES =			\
	usd-media-keys-window.c			\
	usd-media-keys-window.h			\
	test-media-window.c			\
	$(NULL)

test_media_window_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon			\
	-I$(top_srcdir)/plugins/common				\
	-DDATADIR=\""$(datadir)"\"				\
	-DPIXMAPDIR=\""$(pkgdatadir)"\"				\
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"			\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

test_media_window_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(AM_CFLAGS)

test_media_window_LDADD = \
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_DAEMON_LIBS)			\
	$(SETTINGS_PLUGIN_LIBS)			\
	$(XF86MISC_LIBS)			\
	$(GST_LIBS)				\
	-lm

test_media_keys_SOURCES =			\
	usd-media-keys-manager.c		\
	usd-media-keys-manager.h		\
	usd-media-keys-window.h			\
	usd-media-keys-window.c			\
	test-media-keys.c			\
	$(BUILT_SOURCES)			\
	$(NULL)

test_media_keys_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon			\
	-I$(top_srcdir)/plugins/common				\
	-DPIXMAPDIR=\""$(pkgdatadir)"\"				\
	-DGTKBUILDERDIR=\""$(pkgdatadir)"\"			\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

test_media_keys_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
    $(LIBMATEMIXER_CFLAGS)   \
    $(LIBCANBERRA_CFLAGS)   \
	$(AM_CFLAGS)

test_media_keys_LDADD = \
	$(top_builddir)/ukui-settings-daemon/libusd-profile.la		\
	$(top_builddir)/plugins/common/libcommon.la			\
	$(SETTINGS_DAEMON_LIBS)			\
	$(SETTINGS_PLUGIN_LIBS)			\
    $(LIBMATEMIXER_LIBS)         \
    $(LIBCANBERRA_LIBS)         \
	$(XF86MISC_LIBS)			\
	-lm

gtkbuilderdir = $(pkgdatadir)
gtkbuilder_DATA =	\
	acme.ui		\
	$(NULL)

EXTRA_DIST = 				\
	usd-media-keys-manager.xml	\
	usd-marshal.list		\
	$(plugin_in_files)		\
	$(gtkbuilder_DATA)

CLEANFILES = 			\
	$(BUILT_SOURCES)	\
	$(plugin_DATA)

DISTCLEANFILES =		\
	$(plugin_DATA)

@USD_INTLTOOL_PLUGIN_RULE@
