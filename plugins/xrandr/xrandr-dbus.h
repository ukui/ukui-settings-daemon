#ifndef XRANDRDBUS_H
#define XRANDRDBUS_H

#include <QGSettings/qgsettings.h>
#include <QObject>
#include "usd_global_define.h"
#include "xrandr-manager.h"

class xrandrDbus : public QObject
{
    Q_OBJECT
        Q_CLASSINFO("D-Bus Interface",DBUS_XRANDR_INTERFACE)

public:
    xrandrDbus(QObject* parent=0);
    ~xrandrDbus();
     XrandrManager *xrandrManager = nullptr;
public Q_SLOTS:
    int setScreenMode(QString modeName, QString appName);
    int getScreenMode(QString appName);

    int setScreensParam(QString screensParam, QString appName);
    QString getScreensParam(QString appName);

    int sendModeChangeSignal(int screensMode);
    int sendScreensParam(QString screensParam);
Q_SIGNALS:
    //供xrandrManager监听
    void setScreenModeSignal(QString modeName);
    void setScreensParamSignal(QString screensParam);

    //与adaptor一致
    void screensParamChanged(QString screensParam);
    void screenModeChanged(int screenMode);


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
