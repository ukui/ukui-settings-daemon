
noinst_LTLIBRARIES = libcommon.la

libcommon_la_SOURCES = \
	eggaccelerators.c	\
	eggaccelerators.h	\
	usd-keygrab.c		\
	usd-keygrab.h		\
	usd-input-helper.c	\
	usd-input-helper.h	\
	usd-osd-window.c	\
	usd-osd-window.h

libcommon_la_CPPFLAGS = \
	$(AM_CPPFLAGS)

libcommon_la_CFLAGS = \
	$(SETTINGS_PLUGIN_CFLAGS)	\
	$(XINPUT_CFLAGS)		\
	$(AM_CFLAGS)

libcommon_la_LDFLAGS = \
	$(USD_PLUGIN_LDFLAGS) $(XINPUT_LIBS) $(X11_LIBS)

libcommon_la_LIBADD  = \
	$(SETTINGS_PLUGIN_LIBS)		\
	$(XINPUT_LIBS)
