QT += core gui dbus x11extras
CONFIG += c++11 no_keywords link_pkgconfig x11extras debug
CONFIG -= app_bundle

INCLUDEPATH += -I $$PWD/
LIBS += -lukui-com4cxx

PLUGIN_INSTALL_DIRS = $$[QT_INSTALL_LIBS]/ukui-settings-daemon

EXTRA_CFLAGS +=-Wno-date-time

PKGCONFIG += glib-2.0  gio-2.0 libxklavier x11 xrandr xtst atk gdk-3.0 gtk+-3.0 xi
QMAKE_CXXFLAGS +=  -Wno-unused-parameter
SOURCES += \
        $$PWD/clib-syslog.c             \
        $$PWD/QGSettings/qconftype.cpp  \
        $$PWD/QGSettings/qgsettings.cpp \
        $$PWD/rfkillswitch.cpp \
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
        $$PWD/rfkillswitch.h \
        $$PWD/usd_base_class.h \
        $$PWD/usd_global_define.h \
        $$PWD/xeventmonitor.h           \
        $$PWD/eggaccelerators.h         \
        $$PWD/ukui-input-helper.h       \
        $$PWD/ukui-keygrab.h            \
        $$PWD/config.h \
        $$PWD/xrandrouput.h

RESOURCES += \
    $$PWD/ukui_icon.qrc
