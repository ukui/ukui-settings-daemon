NULL =

enabled_plugins =	\
	background	\
	clipboard	\
	housekeeping	\
	keybindings	\
	keyboard	\
	media-keys	\
	mpris		\
	mouse		\
	sharing		\
	sound		\
	xrandr		\
	xrdb		\
	xsettings	\
	$(NULL)

disabled_plugins = $(NULL)

#if SMARTCARD_SUPPORT
#enabled_plugins += smartcard
#else
#disabled_plugins += smartcard
#endif

SUBDIRS = common $(enabled_plugins)
DIST_SUBDIRS = $(SUBDIRS) $(disabled_plugins)
