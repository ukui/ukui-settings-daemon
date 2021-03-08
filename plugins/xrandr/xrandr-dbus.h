#ifndef XRANDRDBUS_H
#define XRANDRDBUS_H

#include <QGSettings/qgsettings.h>
#include <QObject>

class xrandrDbus : public QObject
{
    Q_OBJECT
        Q_CLASSINFO("D-Bus Interface","org.ukui.SettingsDaemon.wayland")

public:
    xrandrDbus(QObject* parent=0);
    ~xrandrDbus();

public Q_SLOTS:
    int x();
    int y();
    int width();
    int height();
    int scale();
    QString priScreenName();
    void activateLauncherMenu();
    int priScreenChanged(int x, int y, int width, int height, QString name);

Q_SIGNALS:
    void screenPrimaryChanged(int x, int y, int width, int height);

public:
    int mX = 0;
    int mY = 0;
    int mWidth = 0;
    int mHeight = 0;
    int mScale = 1;
    QString mName;
    QGSettings *mSession;
    QGSettings *mScreenShot;
};

#endif // XRANDRDBUS_H
