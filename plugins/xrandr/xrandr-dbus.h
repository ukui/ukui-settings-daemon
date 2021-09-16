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
    void initShortKeys();

public Q_SLOTS:
    int x();
    int y();
    int width();
    int height();
    double scale();
    QString priScreenName();
    int priScreenChanged(int x, int y, int width, int height, QString name);
    int setScreenMode(QString modeName, QString appName);
    int getScreenMode(QString appName);

Q_SIGNALS:
    void screenPrimaryChanged(int x, int y, int width, int height);
    void setScreenModeSignal(QString modeName);
    void brightnessDown();
    void brightnessUp();

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
