TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS +=\
    $$PWD/plugins/mpris/mpris.pro \
    $$PWD/plugins/sound/sound.pro \
    $$PWD/plugins/background/background.pro\
    $$PWD/plugins/typing-break/typing-break.pro \
    $$PWD/plugins/a11y-settings/a11y-settings.pro\
    $$PWD/daemon/daemon.pro

include($$PWD/data/data.pri)

OTHER_FILES += \
    $$PWD/README.md\
    $$PWD/LICENSE\
    $$PWD/install.sh
