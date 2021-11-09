#ifndef XEVENTMONITOR_H
#define XEVENTMONITOR_H

#include <QThread>
#include <X11/Xlibint.h>
#include <X11/Xlib.h>
#include <X11/extensions/record.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#include <QX11Info>
// Virtual button codes that are not defined by X11.
#define Button1			1
#define Button2			2
#define Button3			3
#define WheelUp			4
#define WheelDown		5
#define WheelLeft		6
#define WheelRight		7
#define XButton1		8
#define XButton2		9

extern "C"{
#include "clib-syslog.h"
}


class EventMonitor_t : public QThread
{
    Q_OBJECT

public:
    EventMonitor_t(QObject *parent = 0);
    bool winPress=false;
    bool ctrlPress_l=false;
    bool altPress_l=false;
    bool shiftPress_l = false;


    bool ctrlPress_r=false;
    bool altPress_r=false;
    bool shiftPress_r = false;

Q_SIGNALS:
    void buttonPress(int x, int y);
    void buttonDrag(int x, int y);
    void buttonRelease(int x, int y);

    void keyPress(xEvent *code);
    void keyRelease(xEvent *code);

protected:
    bool filterWheelEvent(int detail);
    static void callback(XPointer trash, XRecordInterceptData* data);
    void handleRecordEvent(XRecordInterceptData *);
    void run();

private:
    bool isPress;
};

#endif // XEVENTMONITOR_H

