#ifndef XRANDRDBUS_H
#define XRANDRDBUS_H

#include <QObject>

class xrandrDbus : public QObject
{
    Q_OBJECT
        Q_CLASSINFO("D-Bus Interface","org.ukui.SettingsDaemon.xrandr")

public:
    xrandrDbus(QObject* parent=0);

public Q_SLOTS:
    int x();
    int y();
    int width();
    int height();
    int priScreenChanged(int x, int y, int width, int height);

Q_SIGNALS:
    void screenPrimaryChanged(int x, int y, int width, int height);

public:
    int mX = 0;
    int mY = 0;
    int mWidth = 0;
    int mHeight = 0;
};

#endif // XRANDRDBUS_H
