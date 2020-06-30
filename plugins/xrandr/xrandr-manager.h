#ifndef XRANDRMANAGER_H
#define XRANDRMANAGER_H

#include <QObject>
#include <QTimer>
#include <QFile>
#include <QDebug>
#include <QDomDocument>
#include <QtXml>
#include <QX11Info>
#include <QProcess>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QWidget>
#include <QDesktopWidget>
#include <QMultiMap>
#include <QScreen>

#include<X11/Xlib.h>
#include<X11/extensions/Xrandr.h>

extern "C" {
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-rr.h>
#include <libmate-desktop/mate-rr-config.h>
#include <libmate-desktop/mate-rr-labeler.h>
#include <libmate-desktop/mate-desktop-utils.h>
}

class XrandrManager: public QObject
{
    Q_OBJECT
private:
    XrandrManager();
    XrandrManager(XrandrManager&)=delete;
    XrandrManager&operator=(const XrandrManager&)=delete;
public:
    ~XrandrManager();
    static XrandrManager *XrandrManagerNew();
    bool XrandrManagerStart();
    void XrandrManagerStop();

public Q_SLOTS:
    void StartXrandrIdleCb ();
    //void slotDeviceAdded();

public:
    bool ReadMonitorsXml();
    bool SetScreenSize(Display  *dpy, Window root, int width, int height);
    static void AutoConfigureOutputs (XrandrManager *manager, guint32 timestamp);
    static void OnRandrEvent(MateRRScreen *screen, gpointer data);

private:
    QTimer *time;
    static XrandrManager  *mXrandrManager;
    MateRRScreen *rw_screen;

protected:
    QMultiMap<QString, QString> XmlFileTag; //存放标签的属性值
    QMultiMap<QString, int>     mIntDate;

};

#endif // XRANDRMANAGER_H
