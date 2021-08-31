QT += gui
QT += core widgets
TARGET = housekeeping
TEMPLATE = lib
DEFINES += HOUSEKEPPING_LIBRARY MODULE_NAME=\\\"housekeeping\\\"

CONFIG += c++11 no_keywords link_pkgconfig plugin

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)


PKGCONFIG += gtk+-3.0    glib-2.0     harfbuzz   gmodule-2.0 \
             libxklavier gobject-2.0  gio-2.0    gio-unix-2.0 \
             cairo       cairo-gobject    gsettings-qt

INCLUDEPATH += \
        -I $$PWD/../..                  \
        -I ukui-settings-daemon/

SOURCES += \
    housekeeping-manager.cpp \
    housekeeping-plugin.cpp \
    ldsm-trash-empty.cpp \
    usd-disk-space.cpp \
    usd-ldsm-dialog.cpp

HEADERS += \
    housekeeping-manager.h \
    housekeeping-plugin.h \
    ldsm-trash-empty.h \
    usd-disk-space.h \
    usd-ldsm-dialog.h

housekeeping_lib.path = $${PLUGIN_INSTALL_DIRS}
housekeeping_lib.files = $$OUT_PWD/libhousekeeping.so

INSTALLS += housekeeping_lib

FORMS += \
    ldsm-trash-empty.ui \
    usd-ldsm-dialog.ui

RESOURCES += \
    trash_empty.qrc
