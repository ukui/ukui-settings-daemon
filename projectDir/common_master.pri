QT += core gui dbus x11extras
CONFIG += c++11 no_keywords link_pkgconfig x11extras
CONFIG -= app_bundle

INCLUDEPATH += -I $$PWD/

PLUGIN_INSTALL_DIRS = $$[QT_INSTALL_LIBS]/ukui-settings-daemon

DEFINES += USD_master

EXTRA_CFLAGS +=-Wno-date-time

PKGCONFIG += glib-2.0  gio-2.0 libxklavier x11 xrandr xtst atk gdk-3.0 gtk+-3.0 xi
QMAKE_CXXFLAGS +=  -Wno-unused-parameter
SOURCES += \
        $$PWD/clib-syslog.c             \
        $$PWD/QGSettings/qconftype.cpp  \
        $$PWD/QGSettings/qgsettings.cpp \
        $$PWD/usd_base_class.cpp \
        $$PWD/xeventmonitor.cpp         \
        $$PWD/eggaccelerators.c         \
        $$PWD/ukui-input-helper.c       \
        $$PWD/ukui-keygrab.cpp

HEADERS += \
        $$PWD/clib-syslog.h             \
        $$PWD/plugin-interface.h        \
        $$PWD/QGSettings/qconftype.h    \
        $$PWD/QGSettings/qgsettings.h   \
        $$PWD/usd_base_class.h \
        $$PWD/xeventmonitor.h           \
        $$PWD/eggaccelerators.h         \
        $$PWD/ukui-input-helper.h       \
        $$PWD/ukui-keygrab.h            \
        $$PWD/config.h
