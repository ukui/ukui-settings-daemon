#-------------------------------------------------
#
# Project created by QtCreator 2020-09-10T14:30:00
#
#-------------------------------------------------
QT += gui
QT += core xml widgets x11extras

TEMPLATE = lib
DEFINES += COLOR_LIBRARY

CONFIG += c++11 link_pkgconfig no_keywords plugin app_bundle

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I ukui-settings-daemon/

PKGCONFIG += \
        glib-2.0 \
        gtk+-3.0 \
        colord \
        libgeoclue-2.0 \
        gobject-2.0 \
        libnotify \
        libcanberra \
        libcanberra-gtk3 \
        gio-2.0 \
        mate-desktop-2.0 \
        gnome-desktop-3.0

SOURCES += \
    color-edid.cpp \
    color-manager.cpp \
    color-plugin.cpp \
    color-profiles.cpp \
    color-state.cpp

HEADERS += \
    color-edid.h \
    color-manager.h \
    color-plugin.h \
    color-profiles.h \
    color-state.h

color_lib.path = $${PLUGIN_INSTALL_DIRS}
color_lib.files = $$OUT_PWD/libcolor.so

INSTALLS += color_lib
