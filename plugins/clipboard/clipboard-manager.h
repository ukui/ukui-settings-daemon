#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H
#include "list.h"

#include <QObject>
#include <QThread>

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>


class ClipboardThread;

typedef struct
{
    int                         length;
    int                         format;
    int                         refcount;
    Atom                        target;
    Atom                        type;
    unsigned char*              data;
} TargetData;

typedef struct
{
    int                         offset;
    Atom                        target;
    Atom                        property;
    Window                      requestor;
    TargetData*                 data;
} IncrConversion;

class ClipboardManager : public QObject
{
    Q_OBJECT
public:
    explicit ClipboardManager(QObject *parent = nullptr);
    ~ClipboardManager();

    bool start ();
    bool stop ();

Q_SIGNALS:

private:
    Display*                mDisplay;
    Window                  mWindow;
    Time                    mTimestamp;

    List*                   mContents;
    List*                   mConversions;

    Window                  mRequestor;
    Atom                    mProperty;
    Time                    mTime;

    QThread*                mThread;

    friend ClipboardThread;
    friend void get_property (TargetData* tdata, ClipboardManager* manager);
    friend bool send_incrementally (ClipboardManager* manager, XEvent* xev);
    friend bool receive_incrementally (ClipboardManager* manager, XEvent* xev);
    friend void send_selection_notify (ClipboardManager* manager, bool success);
    friend void convert_clipboard_manager (ClipboardManager* manager, XEvent* xev);
    friend void collect_incremental (IncrConversion* rdata, ClipboardManager* manager);
    friend bool clipboard_manager_process_event(ClipboardManager* manager, XEvent* xev);
    friend void save_targets (ClipboardManager* manager, Atom* save_targets, int nitems);
    friend void convert_clipboard_target (IncrConversion* rdata, ClipboardManager* manager);
    friend void finish_selection_request (ClipboardManager* manager, XEvent* xev, bool success);
    friend void clipboard_manager_watch_cb(ClipboardManager* manager, Window window, bool isStart, long mask, void* cbData);
};

#endif // CLIPBOARDMANAGER_H
