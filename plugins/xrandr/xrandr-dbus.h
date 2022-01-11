#ifndef XRANDRDBUS_H
#define XRANDRDBUS_H

#include <QGSettings/qgsettings.h>
#include <QObject>
#include "usd_global_define.h"


class xrandrDbus : public QObject
{
    Q_OBJECT
        Q_CLASSINFO("D-Bus Interface",DBUS_XRANDR_INTERFACE)

public:
    xrandrDbus(QObject* parent=0);
    ~xrandrDbus();

    void sendModeChangeSignal(int screensMode);
    void sendScreensParamChangeSignal(QString screensParam);

public Q_SLOTS:
    int setScreenMode(QString modeName, QString appName);
    int getScreenMode(QString appName);

    int setScreensParam(QString screensParam, QString appName);
    QString getScreensParam(QString appName);

    void setScreenMap();

    QString controlScreenSlot(const QString &arg);


Q_SIGNALS:
    //供xrandrManager监听
    void setScreenModeSignal(QString modeName);
    void setScreensParamSignal(QString screensParam);

    //与adaptor一致
    void screensParamChanged(QString screensParam);
    void screenModeChanged(int screenMode);

    //控制面板旋转触摸映射
    void controlScreen(QString conRotation);


public:
    int mX = 0;
    int mY = 0;
    int mWidth = 0;
    int mHeight = 0;
    double mScale = 1.0;
    int mScreenMode = 0;
    QString mName;
    QGSettings *mXsettings;
};

#endif // XRANDRDBUS_H
