#-------------------------------------------------
#
# Project created by QtCreator 2020-03-16T09:30:00
#
#-------------------------------------------------
TEMPLATE = subdirs

TRANSLATIONS += daemon/res/i18n/zh_CN.ts


CONFIG += ordered

SUBDIRS += \
    $$PWD/plugins/a11y-keyboard/a11y-keyboard.pro  \
    $$PWD/plugins/a11y-settings/a11y-settings.pro  \
    $$PWD/plugins/auto-brightness/auto-brightness.pro  \
#    $$PWD/plugins/background/background.pro        \ 不开启
    $$PWD/plugins/clipboard/clipboard.pro          \
    $$PWD/plugins/color/color.pro                  \
    $$PWD/plugins/housekeeping/housekeeping.pro    \
    $$PWD/plugins/keyboard/keyboard.pro         \
    $$PWD/plugins/keybindings/keybindings.pro   \
    $$PWD/plugins/media-keys/media-keys.pro     \
    $$PWD/plugins/mouse/mouse.pro               \
    $$PWD/plugins/mpris/mpris.pro               \
    $$PWD/plugins/sound/sound.pro               \
    $$PWD/plugins/sharing/sharing.pro           \
    $$PWD/plugins/tablet-mode/tablet-mode.pro   \
    $$PWD/plugins/xrandr/xrandr.pro             \
    $$PWD/plugins/xrdb/xrdb.pro                 \
    $$PWD/plugins/xinput/xinput.pro             \
    $$PWD/plugins/xsettings/xsettings.pro       \
    $$PWD/plugins/locate-pointer/usd-locate-pointer.pro \
    $$PWD/plugins/kds/kds.pro\
    $$PWD/plugins/authority/authority.pro\
#    $$PWD/plugins/save-param/save-param.pro\
    $$PWD/daemon/daemon.pro


include($$PWD/data/data.pri)

OTHER_FILES += \
    $$PWD/LICENSE \
    $$PWD/README.md 

# Automating generation .qm files from .ts files
CONFIG(release, debug|release) {
    !system($$PWD/translate_generation.sh): error("Failed to generate translation")
}
