QT += gui widgets x11extras dbus sensors KConfigCore KConfigGui

TEMPLATE = lib

CONFIG += c++11 plugin link_pkgconfig

DEFINES += TABLETMODE_LIBRARY

DEFINES += QT_DEPRECATED_WARNINGS MODULE_NAME=\\\"tablet-mode\\\"

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I ukui-settings-daemon/        \

PKGCONFIG += \
            xrandr x11 \
            glib-2.0

SOURCES += \
    tabletMode-manager.cpp \
    tabletMode-plugin.cpp

HEADERS += \
    tabletMode-manager.h \
    tabletMode-plugin.h

tablet_mode_lib.path = $${PLUGIN_INSTALL_DIRS}
tablet_mode_lib.files = $$OUT_PWD/libtablet-mode.so

INSTALLS += tablet_mode_lib
