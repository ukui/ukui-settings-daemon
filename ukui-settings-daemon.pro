TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
    $$PWD/plugins/xrdb/xrdb.pro \
    $$PWD/plugins/mpris/mpris.pro \
    $$PWD/plugins/sound/sound.pro \
    $$PWD/plugins/mouse/mouse.pro \
    $$PWD/plugins/xrandr/xrandr.pro \
    $$PWD/plugins/datetime/datetime.pro \
    $$PWD/plugins/xsettings/xsettings.pro \
    $$PWD/plugins/clipboard/clipboard.pro \
    $$PWD/plugins/background/background.pro \
    $$PWD/plugins/media-keys/media-keys.pro \
    $$PWD/plugins/typing-break/typing-break.pro \
    $$PWD/plugins/a11y-settings/a11y-settings.pro \
    $$PWD/daemon/daemon.pro \

include($$PWD/data/data.pri)

OTHER_FILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/install.sh

