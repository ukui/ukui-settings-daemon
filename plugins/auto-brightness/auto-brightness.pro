QT += gui widgets x11extras dbus sensors KConfigCore KConfigGui

TEMPLATE = lib

CONFIG += c++11 plugin link_pkgconfig

DEFINES += TABLETMODE_LIBRARY

DEFINES += QT_DEPRECATED_WARNINGS

include($$PWD/../../common/common.pri)

INCLUDEPATH += \
        -I $$PWD/../../common/          \
        -I ukui-settings-daemon/        \

PKGCONFIG += \
            xrandr x11 \
            glib-2.0 \
            gsettings-qt

SOURCES += \
    autoBrightness-manager.cpp \
    autoBrightness-plugin.cpp \
    brightThread.cpp

HEADERS += \
    autoBrightness-manager.h \
    autoBrightness-plugin.h \
    brightThread.h

auto_brightness_lib.path = $${PLUGIN_INSTALL_DIRS}
auto_brightness_lib.files = $$OUT_PWD/libauto-brightness.so

INSTALLS += auto_brightness_lib
