NULL =

AM_CPPFLAGS = \
	-DDATADIR=\""$(datadir)"\"				\
	-DUKUI_SETTINGS_LOCALEDIR=\""$(datadir)/locale"\"	\
	-DLIBEXECDIR=\""$(libexecdir)"\" 			\
	-DUKUI_SETTINGS_PLUGINDIR=\""$(plugindir)"\"		\
	$(WARN_CFLAGS)						\
	$(DISABLE_DEPRECATED_CFLAGS)				\
	$(SETTINGS_DAEMON_CFLAGS)				\
	$(LIBNOTIFY_CFLAGS)						\
	$(NULL)

noinst_LTLIBRARIES = 			\
	libusd-profile.la		\
	$(NULL)

libusd_profile_la_SOURCES =		\
	ukui-settings-profile.c	\
	ukui-settings-profile.h	\
	$(NULL)

libusd_profile_la_CPPFLAGS = 		\
	$(AM_CPPFLAGS)
	$(DISABLE_DEPRECATED_CFLAGS)	\
	$(NULL)

libusd_profile_la_CFLAGS =		\
	$(SIMPLE_GREETER_CFLAGS)	\
	$(NULL)

libusd_profile_la_LIBADD =		\
	$(NULL)

libusd_profile_la_LDFLAGS = 	\
	-export-dynamic 	\
	$(NULL)

usddir = $(libexecdir)

usd_PROGRAMS = \
	ukui-settings-daemon

ukui-settings-manager-glue.h: ukui-settings-manager.xml Makefile.am
	$(AM_V_GEN) dbus-binding-tool --prefix=ukui_settings_manager --mode=glib-server $< > $@

ukui-settings-client.h: ukui-settings-manager.xml Makefile.am
	$(AM_V_GEN) dbus-binding-tool --prefix=ukui_settings_manager --mode=glib-client $< > $@

BUILT_SOURCES = 			\
	ukui-settings-manager-glue.h	\
	ukui-settings-client.h		\
	$(NULL)

apidir   = $(includedir)/ukui-settings-daemon
api_DATA = 				\
	ukui-settings-client.h		\
	ukui-settings-plugin.h

ukui_settings_daemon_SOURCES =		\
	main.c				\
	ukui-settings-manager.c	\
	ukui-settings-manager.h	\
	ukui-settings-plugin.c		\
	ukui-settings-plugin.h		\
	ukui-settings-plugin-info.c	\
	ukui-settings-plugin-info.h	\
	ukui-settings-module.c		\
	ukui-settings-module.h		\
	$(NULL)

ukui_settings_daemon_CPPFLAGS = \
	$(AM_CPPFLAGS)

ukui_settings_daemon_CFLAGS = 		\
	$(MATE_DESKTOP_CFLAGS)          \
	$(AM_CFLAGS)

ukui_settings_daemon_LDFLAGS = 	\
	$(AM_LDFLAGS)

ukui_settings_daemon_LDADD = 		\
	libusd-profile.la		\
	$(SETTINGS_DAEMON_LIBS)	\
	$(MATE_DESKTOP_LIBS)    \
	$(LIBNOTIFY_LIBS)

EXTRA_DIST = 				\
	$(Dbusapi_DATA)			\
	ukui-settings-manager.xml	\
	$(NULL)

CLEANFILES = 				\
	$(BUILT_SOURCES)		\
	$(NULL)


# vim: ts=8
