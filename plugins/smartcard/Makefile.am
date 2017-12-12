plugin_LTLIBRARIES = \
	libsmartcard.la

libsmartcard_la_SOURCES = \
	usd-smartcard-plugin.h  \
	usd-smartcard-plugin.c  \
	usd-smartcard.h         \
	usd-smartcard.c         \
	usd-smartcard-manager.h \
	usd-smartcard-manager.c

libsmartcard_la_CPPFLAGS = \
	-I$(top_srcdir)/ukui-settings-daemon \
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\" \
	-DSYSCONFDIR=\""$(sysconfdir)"\" \
	-DLIBDIR=\""$(libdir)"\" \
	-DUSD_SMARTCARD_MANAGER_NSS_DB=\""$(NSS_DATABASE)"\" \
	$(AM_CPPFLAGS)

libsmartcard_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS) \
	$(UKUI_CFLAGS)	\
	$(NSS_CFLAGS)	\
	$(AM_CFLAGS)

libsmartcard_la_LDFLAGS = \
	$(USD_PLUGIN_LDFLAGS)

libsmartcard_la_LIBADD = \
	$(SETTINGS_PLUGIN_LIBS)	\
	$(NSS_LIBS)

@USD_INTLTOOL_PLUGIN_RULE@

plugin_in_files = \
	smartcard.ukui-settings-plugin.in

plugin_DATA = $(plugin_in_files:.ukui-settings-plugin.in=.ukui-settings-plugin)

EXTRA_DIST = \
	$(plugin_in_files)

CLEANFILES = \
	$(plugin_DATA)

DISTCLEANFILES = \
	$(plugin_DATA)
