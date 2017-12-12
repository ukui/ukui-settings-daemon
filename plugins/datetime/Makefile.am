dbus_servicesdir = $(datadir)/dbus-1/system-services
dbus_confdir = $(sysconfdir)/dbus-1/system.d
polkitdir = $(datadir)/polkit-1/actions

dbus_services_in_files = org.ukui.SettingsDaemon.DateTimeMechanism.service.in
polkit_in_files = org.ukui.settingsdaemon.datetimemechanism.policy.in

usd-datetime-mechanism-glue.h: $(srcdir)/usd-datetime-mechanism.xml
	$(AM_V_GEN) dbus-binding-tool \
			--prefix=usd_datetime_mechanism --mode=glib-server	\
			--output=usd-datetime-mechanism-glue.h			\
			$(srcdir)/usd-datetime-mechanism.xml

if HAVE_POLKIT
libexec_PROGRAMS = usd-datetime-mechanism
endif

usd_datetime_mechanism_SOURCES =		\
	usd-datetime-mechanism.c		\
	usd-datetime-mechanism.h		\
	usd-datetime-mechanism-main.c	\
	system-timezone.c			\
	system-timezone.h


if HAVE_POLKIT
BUILT_SOURCES = usd-datetime-mechanism-glue.h
endif

AM_CFLAGS = $(SETTINGS_PLUGIN_CFLAGS) $(POLKIT_CFLAGS)
usd_datetime_mechanism_LDADD = $(POLKIT_LIBS) $(SETTINGS_PLUGIN_LIBS)


if HAVE_POLKIT
dbus_services_DATA = $(dbus_services_in_files:.service.in=.service)

$(dbus_services_DATA): $(dbus_services_in_files)
	$(AM_V_GEN)sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

dbus_conf_DATA = org.ukui.SettingsDaemon.DateTimeMechanism.conf

@INTLTOOL_POLICY_RULE@
polkit_DATA = $(polkit_in_files:.policy.in=.policy)

else
dbus_services_DATA =
dbus_conf_DATA =
polkit_DATA =
endif

EXTRA_DIST =						\
	$(dbus_services_in_files)			\
	org.ukui.SettingsDaemon.DateTimeMechanism.conf	\
	$(polkit_in_files)				\
	usd-datetime-mechanism.xml

CLEANFILES = 		\
	org.ukui.SettingsDaemon.DateTimeMechanism.service	\
	org.ukui.settingsdaemon.datetimemechanism.policy	\
	$(BUILT_SOURCES)
