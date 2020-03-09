TEMPLATE = subdirs

CONFIG += ordered

SUBDIRS +=\
        $$PWD/plugins/background/background.pro\
        $$PWD/daemon/daemon.pro \
        plugins/a11y-settings/a11y-settings.pro \
        plugins/dummy/dummy.pro

