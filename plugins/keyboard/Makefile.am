NULL =

plugin_LTLIBRARIES = \
	libkeyboard.la		\
	$(NULL)

themedir = $(pkgdatadir)/icons/hicolor
size = 64x64
context = devices

iconsdir = $(themedir)/$(size)/$(context)
icons_DATA = \
	  kbd-capslock-off.png kbd-numlock-off.png kbd-scrolllock-off.png \
	  kbd-capslock-on.png kbd-numlock-on.png kbd-scrolllock-on.png

libkeyboard_la_SOURCES = 	\
	usd-keyboard-plugin.h	\
	usd-keyboard-plugin.c	\
	usd-keyboard-manager.h	\
	usd-keyboard-manager.c	\
	usd-keyboard-xkb.h	\
	usd-keyboard-xkb.c	\
	delayed-dialog.h	\
	delayed-dialog.c	\
	$(NULL)

libkeyboard_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon		\
	-DDATADIR=\""$(pkgdatadir)"\"	\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	$(AM_CPPFLAGS)

libkeyboard_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(LIBMATEKBDUI_CFLAGS)		\
	$(AM_CFLAGS)

libkeyboard_la_LDFLAGS = 	\
	$(USD_PLUGIN_LDFLAGS)	\
	$(NULL)

libkeyboard_la_LIBADD  = 	\
	$(SETTINGS_PLUGIN_LIBS)	\
	$(XF86MISC_LIBS)	\
	$(LIBMATEKBDUI_LIBS)	\
	$(NULL)

plugin_in_files = 		\
	keyboard.ukui-settings-plugin.in	\
	$(NULL)

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = 			\
	$(icons_DATA)		\
	$(plugin_in_files)	\
	$(ui_DATA)		\
	$(NULL)

CLEANFILES = 			\
	$(plugin_DATA)		\
	$(NULL)

DISTCLEANFILES =		\
	$(plugin_DATA)		\
	$(NULL)

@USD_INTLTOOL_PLUGIN_RULE@
