#include "xrandr-dbus.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDebug>
#define DBUS_NAME  "org.ukui.SettingsDaemon"
#define DBUS_PATH  "/org/ukui/SettingsDaemon/xrandr"
#define DBUS_INTER "org.ukui.SettingsDaemon.xrandr"

xrandrDbus::xrandrDbus(QObject* parent) :
    QObject(parent){

}

int xrandrDbus::x() {
    return mX;
}
int xrandrDbus::y() {
    return mY;
}
int xrandrDbus::width() {
    return mWidth;
}
int xrandrDbus::height() {
    return mHeight;
}
int xrandrDbus::priScreenChanged(int x, int y, int width, int height)
{
    mX = x;
    mY = y;
    mWidth = width;
    mHeight = height;
    //qDebug()<< x<< y << width << height;
    Q_EMIT screenPrimaryChanged(x, y, width, height);
    return 0;
}
