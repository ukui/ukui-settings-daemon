TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS +=\
        $$PWD/plugins/background/background.pro\
        $$PWD/plugins/a11y-settings/a11y-settings.pro\
        $$PWD/plugins/dummy/dummy.pro\
        $$PWD/daemon/daemon.pro

include($$PWD/data/data.pri)

