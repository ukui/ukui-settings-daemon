QT       += core gui x11extras

LIBS += -lXi -lX11

TEMPLATE = lib
DEFINES += XINPUT_LIBRARY

include($$PWD/../../common/common.pri)

CONFIG += c++11 link_pkgconfig x11extras debug
CONFIG += plugin

PKGCONFIG += x11

DEFINES += QT_DEPRECATED_WARNINGS MODULE_NAME=\\\"xinput\\\"

QMAKE_CXXFLAGS += -Wdeprecated-declarations

INCLUDEPATH += \
        -I $$PWD/../../common/          \
        -I ukui-settings-daemon/

SOURCES += \
        monitorinputtask.cpp \
        xinputmanager.cpp \
        xinputplugin.cpp

HEADERS += \
        monitorinputtask.h \
        xinputmanager.h \
        xinputplugin.h

xinput_lib.path = $${PLUGIN_INSTALL_DIRS}
xinput_lib.files = $$OUT_PWD/libxinput.so

INSTALLS += xinput_lib
