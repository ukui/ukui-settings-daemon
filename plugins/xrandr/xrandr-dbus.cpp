#include "xrandr-dbus.h"
#include <QDBusConnection>
#include <QDBusInterface>
#include <QProcess>
#include <QDebug>
#include <QAction>
#include <KF5/KGlobalAccel/KGlobalAccel>

#include <glib.h>
#include <gio/gio.h>

#include "clib-syslog.h"
#include "usd_base_class.h"
#define DBUS_NAME  "org.ukui.SettingsDaemon"
#define DBUS_PATH  "/org/ukui/SettingsDaemon/wayland"
#define DBUS_INTER "org.ukui.SettingsDaemon.wayland"

#define UKUI_DAEMON_NAME    "ukui-settings-daemon"

xrandrDbus::xrandrDbus(QObject* parent) : QObject(parent)
{
    mXsettings =new QGSettings("org.ukui.SettingsDaemon.plugins.xsettings");
    mScale = mXsettings->get("scaling-factor").toDouble();
    initShortKeys();
}
xrandrDbus::~xrandrDbus()
{
    delete mXsettings;
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

double xrandrDbus::scale(){
    return mScale;
}

QString xrandrDbus::priScreenName(){
    return mName;
}

void xrandrDbus::initShortKeys()
{
    QAction *brightDown= new QAction(this);
    brightDown->setObjectName(QStringLiteral("Brightness down"));
    brightDown->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(brightDown, QList<QKeySequence>{Qt::Key_MonBrightnessDown});
    KGlobalAccel::self()->setShortcut(brightDown, QList<QKeySequence>{Qt::Key_MonBrightnessDown});
    connect(brightDown, &QAction::triggered, this, [this]() {
        Q_EMIT brightnessDown();
    });

    QAction *brightUp = new QAction(this);
    brightUp->setObjectName(QStringLiteral("Brightness Up"));
    brightUp->setProperty("componentName", QStringLiteral(UKUI_DAEMON_NAME));
    KGlobalAccel::self()->setDefaultShortcut(brightUp, QList<QKeySequence>{Qt::Key_MonBrightnessUp});
    KGlobalAccel::self()->setShortcut(brightUp, QList<QKeySequence>{Qt::Key_MonBrightnessUp});
    connect(brightUp, &QAction::triggered, this, [this]() {
        Q_EMIT brightnessUp();
    });
}

int xrandrDbus::priScreenChanged(int x, int y, int width, int height, QString name)
{
    mX = x;
    mY = y;
    mWidth = width;
    mHeight = height;
    mName = name;

    USD_LOG(LOG_DEBUG, "primary screen changed, x = %d, y = %d, width = %d, height = %d",x, y, width, height);
    Q_EMIT screenPrimaryChanged(x, y, width, height);
    return 0;
}

int xrandrDbus::setScreenMode(QString modeName,QString appName){
    USD_LOG(LOG_DEBUG,"change screen :%s, appName:%s",modeName.toLatin1().data(), appName.toLatin1().data());
    Q_EMIT setScreenModeSignal(modeName);
    return true;
}
