TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
    $$PWD/plugins/a11y-keyboard/a11y-keyboard.pro   \
    $$PWD/plugins/a11y-settings/a11y-settings.pro  \
    $$PWD/plugins/background/background.pro     \
    $$PWD/plugins/clipboard/clipboard.pro      \
    $$PWD/plugins/common/common.pro             \
    $$PWD/plugins/datetime/datetime.pro         \
\ #    $$PWD/plugins/dummy/dummy.pro              \
    $$PWD/plugins/housekeeping                  \
    $$PWD/plugins/keyboard/keyboard.pro         \
    $$PWD/plugins/keybindings/keybindings.pro   \
    $$PWD/plugins/media-keys/media-keys.pro     \
    $$PWD/plugins/mouse/mouse.pro               \
    $$PWD/plugins/mpris/mpris.pro               \
    $$PWD/plugins/smartcard/smartcard.pro       \
    $$PWD/plugins/sound/sound.pro              \
    $$PWD/plugins/typing-break/typing-break.pro \
\#    $$PWD/plugins/xrandr/xrandr.pro            \
    $$PWD/plugins/xrdb/xrdb.pro                \
    $$PWD/plugins/xsettings/xsettings.pro      \
    $$PWD/daemon/daemon.pro  \

include($$PWD/data/data.pri)

OTHER_FILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/install.sh

