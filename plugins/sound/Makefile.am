plugin_LTLIBRARIES = \
	libsound.la

libsound_la_SOURCES = \
	usd-sound-plugin.h \
	usd-sound-plugin.c \
	usd-sound-manager.h \
	usd-sound-manager.c

libsound_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon \
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libsound_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS) \
	$(UKUI_CFLAGS)	\
	$(PULSE_CFLAGS)	\
	$(AM_CFLAGS)

libsound_la_LDFLAGS = \
	$(USD_PLUGIN_LDFLAGS)

libsound_la_LIBADD = \
	$(SETTINGS_PLUGIN_LIBS)	\
	$(PULSE_LIBS)

plugin_in_files = \
	sound.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = \
	$(plugin_in_files)

CLEANFILES = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)

@USD_INTLTOOL_PLUGIN_RULE@
