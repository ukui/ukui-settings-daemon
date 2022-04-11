#-------------------------------------------------
#
# Project created by QtCreator 2020-06-16T09:30:00
#
#-------------------------------------------------
QT += gui widgets svg x11extras dbus KGlobalAccel KWindowSystem
TEMPLATE = lib
CONFIG += c++11 plugin link_pkgconfig
CONFIG -= app_bundle

PKGCONFIG += \
    glib-2.0 \
    gio-2.0 \
    gtk+-3.0 \
    gsettings-qt \
    kscreen2    \

   # libmatemixer

LIBS += \
    -lX11 -lXi -lpulse

include($$PWD/../../common/common.pri)

DEFINES += QT_DEPRECATED_WARNINGS HAVE_X11_EXTENSIONS_XKB_H _FORTIFY_SOURCE=2 MODULE_NAME=\\\"mediakeys\\\"

SOURCES += \
        devicewindow.cpp     \
        pulseaudiomanager.cpp \
        volumewindow.cpp \
        mediakey-manager.cpp \
        mediakey-plugin.cpp \
        xEventMonitor.cpp

HEADERS += \
    acme.h \
    devicewindow.h      \
    pulseaudiomanager.h \
    volumewindow.h      \
    mediakey-manager.h  \
    mediakey-plugin.h   \
    xEventMonitor.h

FORMS += \
    devicewindow.ui \
    volumewindow.ui

DISTFILES += \
    media-keys.ukui-settings-plugin.in

media_keys_lib.path = $${PLUGIN_INSTALL_DIRS}
media_keys_lib.files = $$OUT_PWD/libmedia-keys.so

INSTALLS += media_keys_lib
