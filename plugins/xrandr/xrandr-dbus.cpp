#include "xrandr-dbus.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QProcess>
#include <QDebug>

#include <glib.h>
#include <gio/gio.h>

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
    QGSettings *xsettings =new QGSettings("org.ukui.SettingsDaemon.plugins.xsettings");
    if(xsettings){
        mScale = xsettings->get("scaling-factor").toInt();
        delete xsettings;
    }
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

int xrandrDbus::scale(){
    return mScale;
}

QString xrandrDbus::priScreenName(){
    return mName;
}

void executeCommand(const QString& command)
{
    QString cmd = command;
    char   **argv;
    int     argc;
    bool    retval;

    if(!cmd.isEmpty()){
        if (g_shell_parse_argv (cmd.toLatin1().data(), &argc, &argv, NULL)) {
            retval = g_spawn_async (g_get_home_dir (),
                                    argv,
                                    NULL,
                                    G_SPAWN_SEARCH_PATH,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
            g_strfreev (argv);
        }
    }
}

void xrandrDbus::activateLauncherMenu() {
    bool session = mSession->get(WIN_KEY).toBool();
    bool screenshot = mScreenShot->get(RUNNING_KEY).toBool();
    if(!(session || screenshot)){
            executeCommand ("ukui-menu");
    }
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
