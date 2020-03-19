TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS += \
    $$PWD/plugins/mpris/mpris.pro \
    $$PWD/plugins/sound/sound.pro \
    $$PWD/plugins/datetime/datetime.pro \
    $$PWD/plugins/clipboard/clipboard.pro \
    $$PWD/plugins/background/background.pro \
    $$PWD/plugins/typing-break/typing-break.pro \
<<<<<<< HEAD
    $$PWD/plugins/a11y-settings/a11y-settings.pro\
    $$PWD/plugins/common/common.pro\
    $$PWD/daemon/daemon.pro
=======
    $$PWD/plugins/a11y-settings/a11y-settings.pro \
    $$PWD/daemon/daemon.pro \
>>>>>>> 9224dea0bba62c7acf0cf89612ecac63d1ddb165

include($$PWD/data/data.pri)

OTHER_FILES += \
    $$PWD/LICENSE \
    $$PWD/README.md \
    $$PWD/install.sh
