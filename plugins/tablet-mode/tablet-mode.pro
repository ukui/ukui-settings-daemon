QT += gui widgets x11extras dbus

TEMPLATE = lib

CONFIG += c++11 plugin link_pkgconfig

DEFINES += TABLETMODE_LIBRARY

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I $$PWD/../../common/          \
        -I ukui-settings-daemon/        \

PKGCONFIG += \
            xrandr x11 gtk+-3.0 \
            glib-2.0 mate-desktop-2.0

SOURCES += \
    tablet-mode-plugin.cpp

HEADERS += \
    tablet-mode-plugin.h

tablet_mode_lib.path = $${PLUGIN_INSTALL_DIRS}
tablet_mode_lib.files = $$PWD/libtablet-mode.so

INSTALLS += tablet_mode_lib
