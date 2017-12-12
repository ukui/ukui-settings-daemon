icondir = $(datadir)/icons/hicolor
context = apps

BUILT_SOURCES =				\
	usd-xrandr-manager-glue.h

ICON_FILES = 			\
	usd-xrandr-16.png	\
	usd-xrandr-22.png	\
	usd-xrandr-24.png	\
	usd-xrandr-32.png	\
	usd-xrandr.svg

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(sysconfdir)/ukui-settings-daemon/xrandr
	$(mkinstalldirs) $(DESTDIR)$(icondir)/16x16/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/22x22/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/24x24/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/32x32/$(context)
	$(mkinstalldirs) $(DESTDIR)$(icondir)/scalable/$(context)
	$(INSTALL_DATA) $(srcdir)/usd-xrandr-16.png $(DESTDIR)$(icondir)/16x16/$(context)/usd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/usd-xrandr-22.png $(DESTDIR)$(icondir)/22x22/$(context)/usd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/usd-xrandr-24.png $(DESTDIR)$(icondir)/24x24/$(context)/usd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/usd-xrandr-32.png $(DESTDIR)$(icondir)/32x32/$(context)/usd-xrandr.png
	$(INSTALL_DATA) $(srcdir)/usd-xrandr.svg $(DESTDIR)$(icondir)/scalable/$(context)/usd-xrandr.svg

uninstall-local:
	rm -f $(DESTDIR)$(icondir)/16x16/$(context)/usd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/22x22/$(context)/usd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/24x24/$(context)/usd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/32x32/$(context)/usd-xrandr.png
	rm -f $(DESTDIR)$(icondir)/scalable/$(context)/usd-xrandr.svg

plugin_LTLIBRARIES = \
	libxrandr.la

usd-xrandr-manager-glue.h: usd-xrandr-manager.xml Makefile
	$(AM_V_GEN) dbus-binding-tool --prefix=usd_xrandr_manager --mode=glib-server $< > xgen-$(@F) \
	&& ( cmp -s xgen-$(@F) $@ || cp xgen-$(@F) $@ ) \
	&& rm -f xgen-$(@F)

libxrandr_la_SOURCES = 		\
	$(BUILT_SOURCES)	\
	usd-xrandr-plugin.h	\
	usd-xrandr-plugin.c	\
	usd-xrandr-manager.h	\
	usd-xrandr-manager.c

libxrandr_la_CPPFLAGS =						\
	-I$(top_srcdir)/ukui-settings-daemon			\
	-DBINDIR=\"$(bindir)\"					\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	$(AM_CPPFLAGS)

libxrandr_la_CFLAGS =			\
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(LIBNOTIFY_CFLAGS)		\
	$(MATE_DESKTOP_CFLAGS)		\
	$(AM_CFLAGS)

libxrandr_la_LDFLAGS = 			\
	$(USD_PLUGIN_LDFLAGS)

libxrandr_la_LIBADD  =			\
	$(SETTINGS_PLUGIN_LIBS)		\
	$(LIBNOTIFY_LIBS)		\
	$(MATE_DESKTOP_LIBS)

plugin_in_files =			\
	xrandr.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = $(plugin_in_files) $(ICON_FILES) usd-xrandr-manager.xml
CLEANFILES = $(plugin_DATA) $(BUILT_SOURCES)
DISTCLEANFILES = $(plugin_DATA)

@USD_INTLTOOL_PLUGIN_RULE@
