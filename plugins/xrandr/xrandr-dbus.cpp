#include "xrandr-dbus.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QProcess>
#include <QDebug>

#define DBUS_NAME  "org.ukui.SettingsDaemon"
#define DBUS_PATH  "/org/ukui/SettingsDaemon/wayland"
#define DBUS_INTER "org.ukui.SettingsDaemon.wayland"

#define SESSION_SCHEMA      "org.ukui.session"
#define WIN_KEY             "win-key-release"

#define SCREENSHOT_SCHEMA   "org.ukui.screenshot"
#define RUNNING_KEY         "isrunning"

xrandrDbus::xrandrDbus(QObject* parent) :
    QObject(parent){
    mSession = new QGSettings(SESSION_SCHEMA);
    mScreenShot = new QGSettings(SCREENSHOT_SCHEMA);
}
xrandrDbus::~xrandrDbus()
{
    delete mSession;
    delete mScreenShot;
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

QString xrandrDbus::priScreenName(){
    return mName;
}

void xrandrDbus::activateLauncherMenu() {
    bool session = mSession->get(WIN_KEY).toBool();
    bool screenshot = mScreenShot->get(RUNNING_KEY).toBool();
    if(!(session || screenshot))
        QProcess::execute("ukui-menu");
}

int xrandrDbus::priScreenChanged(int x, int y, int width, int height, QString name)
{
    mX = x;
    mY = y;
    mWidth = width;
    mHeight = height;
    mName = name;
    //qDebug()<< x<< y << width << height;
    Q_EMIT screenPrimaryChanged(x, y, width, height);
    return 0;
}
