#include "clipboard-manager.h"
#include "clib-syslog.h"

#include <glib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "xutils.h"
#include "list.h"

void target_data_unref (TargetData *data);
int clipboard_bytes_per_item (int format);
void conversion_free (IncrConversion* rdata);
TargetData* target_data_ref (TargetData *data);
int find_content_type (TargetData* tdata, Atom type);
int find_content_target (TargetData* tdata, Atom target);
void convert_clipboard (ClipboardManager* manager, XEvent* xev);
void get_property (TargetData* tdata, ClipboardManager* manager);
bool send_incrementally (ClipboardManager* manager, XEvent* xev);
int find_conversion_requestor (IncrConversion* rdata, XEvent* xev);
bool receive_incrementally (ClipboardManager* manager, XEvent* xev);
void send_selection_notify (ClipboardManager* manager, bool success);
void convert_clipboard_manager (ClipboardManager* manager, XEvent* xev);
void collect_incremental (IncrConversion* rdata, ClipboardManager* manager);
bool clipboard_manager_process_event(ClipboardManager* manager, XEvent* xev);
void save_targets (ClipboardManager* manager, Atom* save_targets, int nitems);
void convert_clipboard_target (IncrConversion* rdata, ClipboardManager* manager);
void finish_selection_request (ClipboardManager* manager, XEvent* xev, bool success);
GdkFilterReturn clipboard_manager_event_filter (GdkXEvent* xevent, GdkEvent* event, ClipboardManager* manager);
void clipboard_manager_watch_cb(ClipboardManager* manager, Window window, bool isStart, long mask, void* cbData);

class ClipboardThread : public QThread
{
    Q_OBJECT
public:
    explicit ClipboardThread(ClipboardManager*m, QObject* partent) : QThread(partent) {
        mClipboardManager = m;
    }
    ~ClipboardThread() {}
    void run() override {
        while (mExit) {
            XClientMessageEvent xev;
            init_atoms (mClipboardManager->mDisplay);
            /* check if there is a clipboard manager running */
            if (XGetSelectionOwner (mClipboardManager->mDisplay, XA_CLIPBOARD_MANAGER)) {
                CT_SYSLOG(LOG_ERR, "Clipboard manager is already running.");
                mExit = false;
                return;
            }

            mClipboardManager->mContents = nullptr;
            mClipboardManager->mConversions = nullptr;
            mClipboardManager->mRequestor = None;
            mClipboardManager->mWindow = XCreateSimpleWindow (mClipboardManager->mDisplay,
                      DefaultRootWindow (mClipboardManager->mDisplay), 0, 0, 10, 10, 0,
                      WhitePixel (mClipboardManager->mDisplay, DefaultScreen (mClipboardManager->mDisplay)),
                      WhitePixel (mClipboardManager->mDisplay, DefaultScreen (mClipboardManager->mDisplay)));

            clipboard_manager_watch_cb (mClipboardManager, mClipboardManager->mWindow, True, PropertyChangeMask, NULL);

            XSelectInput (mClipboardManager->mDisplay, mClipboardManager->mWindow, PropertyChangeMask);
            mClipboardManager->mTimestamp = get_server_time (mClipboardManager->mDisplay, mClipboardManager->mWindow);

            XSetSelectionOwner (mClipboardManager->mDisplay, XA_CLIPBOARD_MANAGER, mClipboardManager->mWindow, mClipboardManager->mTimestamp);

            /* Check to see if we managed to claim the selection. If not, we treat it as if we got it then immediately lost it */
            if (XGetSelectionOwner (mClipboardManager->mDisplay, XA_CLIPBOARD_MANAGER) == mClipboardManager->mWindow) {
                xev.type = ClientMessage;
                xev.window = DefaultRootWindow (mClipboardManager->mDisplay);
                xev.message_type = XA_MANAGER;
                xev.format = 32;
                xev.data.l[0] = mClipboardManager->mTimestamp;
                xev.data.l[1] = XA_CLIPBOARD_MANAGER;
                xev.data.l[2] = mClipboardManager->mWindow;
                xev.data.l[3] = 0;      /* manager specific data */
                xev.data.l[4] = 0;      /* manager specific data */

                XSendEvent (mClipboardManager->mDisplay, DefaultRootWindow (mClipboardManager->mDisplay), False, StructureNotifyMask, (XEvent *)&xev);
            } else {
                clipboard_manager_watch_cb (mClipboardManager, mClipboardManager->mWindow, False, 0, NULL);
                /* FIXME: manager->priv->terminate (manager->priv->cb_data); */
            }
        }
    }

private:
    ClipboardManager*                   mClipboardManager;
    bool                                mExit;

};

ClipboardManager::ClipboardManager(QObject *parent) : QObject(parent)
{
    mDisplay = gdk_x11_display_get_xdisplay(gdk_display_get_default());
    mThread = new ClipboardThread(this, this);
}

ClipboardManager::~ClipboardManager()
{
    delete mThread;
}

bool ClipboardManager::start()
{
    mThread->start(QThread::LowestPriority);
}

bool ClipboardManager::stop()
{
    mThread->quit();
    clipboard_manager_watch_cb (this, mWindow, FALSE, 0, NULL);
    XDestroyWindow (mDisplay, mWindow);

    list_foreach (mConversions, (Callback) conversion_free, NULL);
    list_free (mConversions);

    list_foreach (mContents, (Callback) target_data_unref, NULL);
    list_free (mContents);
}

void clipboard_manager_watch_cb(ClipboardManager* manager, Window window, bool isStart, long, void*)
{
    GdkWindow*                  gdkwin;
    GdkDisplay*                 display;

    display = gdk_display_get_default ();
    gdkwin = gdk_x11_window_lookup_for_display (display, window);

    if (isStart) {
        if (gdkwin == NULL) {
            gdkwin = gdk_x11_window_foreign_new_for_display (display, window);
        } else {
            g_object_ref (gdkwin);
        }

        gdk_window_add_filter (gdkwin, (GdkFilterFunc)clipboard_manager_event_filter, manager);
    } else {
        if (gdkwin == NULL) {
            return;
        }

        gdk_window_remove_filter (gdkwin, (GdkFilterFunc)clipboard_manager_event_filter, manager);
        g_object_unref (gdkwin);
    }
}

void conversion_free (IncrConversion* rdata)
{
    if (rdata->data) {
        target_data_unref (rdata->data);
    }
    free (rdata);
}

void target_data_unref (TargetData *data)
{
    data->refcount--;
    if (data->refcount == 0) {
        free (data->data);
        free (data);
    }
}

GdkFilterReturn clipboard_manager_event_filter (GdkXEvent* xevent, GdkEvent*, ClipboardManager* manager)
{
    if (clipboard_manager_process_event (manager, (XEvent *)xevent)) {
        return GDK_FILTER_REMOVE;
    } else {
        return GDK_FILTER_CONTINUE;
    }
}

bool clipboard_manager_process_event(ClipboardManager* manager, XEvent* xev)
{
    int                 format;
    Atom                type;
    Atom*               targets;
    unsigned long       nitems;
    unsigned long       remaining;

    targets = nullptr;

    switch (xev->xany.type) {
    case DestroyNotify:
        if (xev->xdestroywindow.window == manager->mRequestor) {
            list_foreach (manager->mContents, (Callback)target_data_unref, nullptr);
            list_free (manager->mContents);
            manager->mContents = nullptr;
            clipboard_manager_watch_cb (manager, manager->mRequestor, false, 0, nullptr);
            manager->mRequestor = None;
        }
        break;
    case PropertyNotify:
        if (xev->xproperty.state == PropertyNewValue) {
            return receive_incrementally (manager, xev);
        } else {
            return send_incrementally (manager, xev);
        }
    case SelectionClear:
        if (xev->xany.window != manager->mWindow) return false;
        if (xev->xselectionclear.selection == XA_CLIPBOARD_MANAGER) {
            /* We lost the manager selection */
            if (manager->mContents) {
                list_foreach (manager->mContents, (Callback)target_data_unref, nullptr);
                list_free (manager->mContents);
                manager->mContents = nullptr;
                XSetSelectionOwner (manager->mDisplay, XA_CLIPBOARD, None, manager->mTime);
            }

            return True;
        }

        if (xev->xselectionclear.selection == XA_CLIPBOARD) {
            /* We lost the clipboard selection */
            list_foreach (manager->mContents, (Callback)target_data_unref, nullptr);
            list_free (manager->mContents);
            manager->mContents = nullptr;
            clipboard_manager_watch_cb (manager, manager->mRequestor, false, 0, nullptr);
            manager->mRequestor = None;
            return true;
        }
        break;
    case SelectionNotify:
        if (xev->xany.window != manager->mWindow) return false;

        if (xev->xselection.selection == XA_CLIPBOARD) {
            /* a CLIPBOARD conversion is done */
            if (xev->xselection.property == XA_TARGETS) {
                XGetWindowProperty (xev->xselection.display, xev->xselection.requestor, xev->xselection.property,
                            0, 0x1FFFFFFF, True, XA_ATOM, &type, &format, &nitems, &remaining, (unsigned char **) &targets);
                save_targets (manager, targets, nitems);
            } else if (xev->xselection.property == XA_MULTIPLE) {
                List *tmp;

                tmp = list_copy (manager->mContents);
                list_foreach (tmp, (Callback) get_property, manager);
                list_free (tmp);

                manager->mTime = xev->xselection.time;
                XSetSelectionOwner (manager->mDisplay, XA_CLIPBOARD, manager->mWindow, manager->mTime);

                if (manager->mProperty != None)
                    XChangeProperty (manager->mDisplay, manager->mRequestor, manager->mProperty,
                                     XA_ATOM, 32, PropModeReplace, (unsigned char *)&XA_NULL, 1);

                if (!list_find (manager->mContents, (ListFindFunc)find_content_type, (void *)XA_INCR)) {
                    /* all transfers done */
                    send_selection_notify (manager, True);
                    clipboard_manager_watch_cb (manager, manager->mRequestor, false, 0, nullptr);
                    manager->mRequestor = None;
                }
            } else if (xev->xselection.property == None) {
                send_selection_notify (manager, false);
                clipboard_manager_watch_cb (manager, manager->mRequestor, false, 0, nullptr);
                manager->mRequestor = None;
            }
            return true;
        }
        break;
    case SelectionRequest:
        if (xev->xany.window != manager->mWindow) {
            return false;
        }
        if (xev->xselectionrequest.selection == XA_CLIPBOARD_MANAGER) {
            convert_clipboard_manager (manager, xev);
            return true;
        } else if (xev->xselectionrequest.selection == XA_CLIPBOARD) {
            convert_clipboard (manager, xev);
            return true;
        }
        break;
    default: ;
    }

    return false;
}

bool receive_incrementally (ClipboardManager* manager, XEvent* xev)
{
    List*                       list;
    TargetData*                 tdata;
    Atom                        type;
    int                         format;
    unsigned char*              data;
    unsigned long               length, nitems, remaining;

    if (xev->xproperty.window != manager->mWindow) return false;

    list = list_find (manager->mContents, (ListFindFunc) find_content_target, (void *) xev->xproperty.atom);
    if (!list) return false;
    tdata = (TargetData *) list->data;
    if (tdata->type != XA_INCR) return false;
    XGetWindowProperty (xev->xproperty.display, xev->xproperty.window, xev->xproperty.atom, 0, 0x1FFFFFFF, True, AnyPropertyType, &type, &format, &nitems, &remaining, &data);
    length = nitems * clipboard_bytes_per_item (format);
    if (length == 0) {
        tdata->type = type;
        tdata->format = format;
        if (!list_find (manager->mContents, (ListFindFunc) find_content_type, (void *)XA_INCR)) {
            /* all incremental transfers done */
            send_selection_notify (manager, True);
            manager->mRequestor = None;
        }
        XFree (data);
    } else {
        if (!tdata->data) {
            tdata->data = data;
            tdata->length = length;
        } else {
            tdata->data = (unsigned char*)realloc (tdata->data, tdata->length + length + 1);
            memcpy (tdata->data + tdata->length, data, length + 1);
            tdata->length += length;
            XFree (data);
        }
    }
    return True;
}

bool send_incrementally (ClipboardManager* manager, XEvent* xev)
{
    List*                               list;
    unsigned long                       length;
    unsigned long                       items;
    unsigned char*                      data;
    IncrConversion*                     rdata;

    list = list_find (manager->mConversions, (ListFindFunc) find_conversion_requestor, xev);
    if (list == NULL) return false;
    rdata = (IncrConversion *) list->data;
    data = rdata->data->data + rdata->offset;
    length = rdata->data->length - rdata->offset;
    if (length > SELECTION_MAX_SIZE) length = SELECTION_MAX_SIZE;
    rdata->offset += length;
    items = length / clipboard_bytes_per_item (rdata->data->format);
    XChangeProperty (manager->mDisplay, rdata->requestor, rdata->property, rdata->data->type, rdata->data->format, PropModeAppend, data, items);

    if (length == 0) {
        manager->mConversions = list_remove (manager->mConversions, rdata);
        conversion_free (rdata);
    }

    return true;
}

void save_targets (ClipboardManager* manager, Atom* save_targets, int nitems)
{
    int                                 nout, i;
    Atom                                *multiple;
    TargetData                          *tdata;

    multiple = (Atom *) malloc (2 * nitems * sizeof (Atom));

    nout = 0;
    for (i = 0; i < nitems; i++) {
            if (save_targets[i] != XA_TARGETS &&
                save_targets[i] != XA_MULTIPLE &&
                save_targets[i] != XA_DELETE &&
                save_targets[i] != XA_INSERT_PROPERTY &&
                save_targets[i] != XA_INSERT_SELECTION &&
                save_targets[i] != XA_PIXMAP) {
                    tdata = (TargetData *) malloc (sizeof (TargetData));
                    tdata->data = NULL;
                    tdata->length = 0;
                    tdata->target = save_targets[i];
                    tdata->type = None;
                    tdata->format = 0;
                    tdata->refcount = 1;
                    manager->mContents = list_prepend (manager->mContents, tdata);
                    multiple[nout++] = save_targets[i];
                    multiple[nout++] = save_targets[i];
            }
    }

    XFree (save_targets);

    XChangeProperty (manager->mDisplay, manager->mWindow, XA_MULTIPLE, XA_ATOM_PAIR, 32, PropModeReplace, (const unsigned char *) multiple, nout);
    free (multiple);

    XConvertSelection (manager->mDisplay, XA_CLIPBOARD, XA_MULTIPLE, XA_MULTIPLE, manager->mWindow, manager->mTime);
}

void get_property (TargetData* tdata, ClipboardManager* manager)
{
    Atom                                type;
    int                                 format;
    unsigned long                       length;
    unsigned long                       remaining;
    unsigned char*                      data;

    XGetWindowProperty (manager->mDisplay, manager->mWindow, tdata->target, 0, 0x1FFFFFFF, true, AnyPropertyType, &type, &format, &length, &remaining, &data);

    if (type == None) {
        manager->mContents = list_remove (manager->mContents, tdata);
        free (tdata);
    } else if (type == XA_INCR) {
        tdata->type = type;
        tdata->length = 0;
        XFree (data);
    } else {
        tdata->type = type;
        tdata->data = data;
        tdata->length = length * clipboard_bytes_per_item (format);
        tdata->format = format;
    }
}

int find_content_type (TargetData* tdata, Atom type)
{
    return tdata->type == type;
}

void send_selection_notify (ClipboardManager* manager, bool success)
{
    XSelectionEvent notify;

    notify.type = SelectionNotify;
    notify.serial = 0;
    notify.send_event = True;
    notify.display = manager->mDisplay;
    notify.requestor = manager->mRequestor;
    notify.selection = XA_CLIPBOARD_MANAGER;
    notify.target = XA_SAVE_TARGETS;
    notify.property = success ? manager->mProperty : None;
    notify.time = manager->mTime;

    gdk_error_trap_push ();
    XSendEvent (manager->mDisplay, manager->mRequestor, false, NoEventMask, (XEvent *)&notify);
    XSync (manager->mDisplay, false);
    gdk_error_trap_pop_ignored ();
}

void convert_clipboard_manager (ClipboardManager* manager, XEvent* xev)
{
    Atom          type = None;
    int           format;
    unsigned long nitems;
    unsigned long remaining;
    Atom         *targets = NULL;

    if (xev->xselectionrequest.target == XA_SAVE_TARGETS) {
        if (manager->mRequestor != None || manager->mContents != nullptr) {
            /* We're in the middle of a conversion request, or own the CLIPBOARD already */
            finish_selection_request (manager, xev, False);
        } else {
            gdk_error_trap_push ();
            clipboard_manager_watch_cb (manager, xev->xselectionrequest.requestor, true, StructureNotifyMask, nullptr);
            XSelectInput (manager->mDisplay, xev->xselectionrequest.requestor, StructureNotifyMask);
            XSync (manager->mDisplay, false);

            if (gdk_error_trap_pop () != Success) return;
            gdk_error_trap_push ();

            if (xev->xselectionrequest.property != None) {
                XGetWindowProperty (manager->mDisplay, xev->xselectionrequest.requestor, xev->xselectionrequest.property,
                                    0, 0x1FFFFFFF, False, XA_ATOM, &type, &format, &nitems, &remaining, (unsigned char **) &targets);
                if (gdk_error_trap_pop () != Success) {
                    if (targets) XFree (targets);
                    return;
                }
            }

            manager->mRequestor = xev->xselectionrequest.requestor;
            manager->mProperty = xev->xselectionrequest.property;
            manager->mTime = xev->xselectionrequest.time;

            if (type == None)
                XConvertSelection (manager->mDisplay, XA_CLIPBOARD, XA_TARGETS, XA_TARGETS, manager->mWindow, manager->mTime);
            else
                save_targets (manager, targets, nitems);
        }
    } else if (xev->xselectionrequest.target == XA_TIMESTAMP) {
        XChangeProperty (manager->mDisplay, xev->xselectionrequest.requestor, xev->xselectionrequest.property,
                         XA_INTEGER, 32, PropModeReplace, (unsigned char *) &manager->mTimestamp, 1);
        finish_selection_request (manager, xev, true);
    } else if (xev->xselectionrequest.target == XA_TARGETS) {
        int  n_targets = 0;
        Atom targets[3];

        targets[n_targets++] = XA_TARGETS;
        targets[n_targets++] = XA_TIMESTAMP;
        targets[n_targets++] = XA_SAVE_TARGETS;

        XChangeProperty (manager->mDisplay, xev->xselectionrequest.requestor, xev->xselectionrequest.property,
                         XA_ATOM, 32, PropModeReplace, (unsigned char *) targets, n_targets);
        finish_selection_request (manager, xev, true);
    } else {
        finish_selection_request (manager, xev, false);
    }
}

void convert_clipboard (ClipboardManager* manager, XEvent* xev)
{
    int                                 format;
    unsigned long                       nitems;
    unsigned long                       remaining;
    List*                               list;
    List*                               conversions;
    Atom                                type;
    Atom*                               multiple;
    IncrConversion*                     rdata;

    conversions = NULL;
    type = None;

    if (xev->xselectionrequest.target == XA_MULTIPLE) {
        XGetWindowProperty (xev->xselectionrequest.display, xev->xselectionrequest.requestor, xev->xselectionrequest.property, 0, 0x1FFFFFFF, False, XA_ATOM_PAIR,
                            &type, &format, &nitems, &remaining, (unsigned char **) &multiple);
        if (type != XA_ATOM_PAIR || nitems == 0) {
            if (multiple) free (multiple);
            return;
        }

        for (unsigned long i = 0; i < nitems; i += 2) {
            rdata = (IncrConversion *) malloc (sizeof (IncrConversion));
            rdata->requestor = xev->xselectionrequest.requestor;
            rdata->target = multiple[i];
            rdata->property = multiple[i+1];
            rdata->data = NULL;
            rdata->offset = -1;
            conversions = list_prepend (conversions, rdata);
        }
    } else {
        multiple = NULL;

        rdata = (IncrConversion *) malloc (sizeof (IncrConversion));
        rdata->requestor = xev->xselectionrequest.requestor;
        rdata->target = xev->xselectionrequest.target;
        rdata->property = xev->xselectionrequest.property;
        rdata->data = NULL;
        rdata->offset = -1;
        conversions = list_prepend (conversions, rdata);
    }

    list_foreach (conversions, (Callback) convert_clipboard_target, manager);

    if (conversions->next == NULL && ((IncrConversion *) conversions->data)->property == None) {
        finish_selection_request (manager, xev, False);
    } else {
        if (multiple) {
            int i = 0;
            for (list = conversions; list; list = list->next) {
                    rdata = (IncrConversion *)list->data;
                    multiple[i++] = rdata->target;
                    multiple[i++] = rdata->property;
            }
            XChangeProperty (xev->xselectionrequest.display, xev->xselectionrequest.requestor,
                             xev->xselectionrequest.property, XA_ATOM_PAIR, 32, PropModeReplace, (unsigned char *) multiple, nitems);
        }
        finish_selection_request (manager, xev, True);
    }

    list_foreach (conversions, (Callback) collect_incremental, manager);
    list_free (conversions);

    if (multiple) free (multiple);
}

void finish_selection_request (ClipboardManager* manager, XEvent* xev, bool success)
{
    XSelectionEvent notify;

    notify.type = SelectionNotify;
    notify.serial = 0;
    notify.send_event = true;
    notify.display = xev->xselectionrequest.display;
    notify.requestor = xev->xselectionrequest.requestor;
    notify.selection = xev->xselectionrequest.selection;
    notify.target = xev->xselectionrequest.target;
    notify.property = success ? xev->xselectionrequest.property : None;
    notify.time = xev->xselectionrequest.time;

    gdk_error_trap_push ();
    XSendEvent (xev->xselectionrequest.display, xev->xselectionrequest.requestor, false, NoEventMask, (XEvent *) &notify);
    XSync (manager->mDisplay, false);
    gdk_error_trap_pop_ignored ();
}

int find_content_target (TargetData* tdata, Atom target)
{
    return tdata->target == target;
}

int clipboard_bytes_per_item (int format)
{
    switch (format) {
    case 8: return sizeof (char);
    case 16: return sizeof (short);
    case 32: return sizeof (long);
    default: ;
    }

    return 0;
}

int find_conversion_requestor (IncrConversion* rdata, XEvent* xev)
{
    return (rdata->requestor == xev->xproperty.window && rdata->property == xev->xproperty.atom);
}

void convert_clipboard_target (IncrConversion* rdata, ClipboardManager* manager)
{
    TargetData       *tdata;
    Atom             *targets;
    int               n_targets;
    List             *list;
    unsigned long     items;
    XWindowAttributes atts;

    if (rdata->target == XA_TARGETS) {
        n_targets = list_length (manager->mContents) + 2;
        targets = (Atom *) malloc (n_targets * sizeof (Atom));

        n_targets = 0;

        targets[n_targets++] = XA_TARGETS;
        targets[n_targets++] = XA_MULTIPLE;

        for (list = manager->mContents; list; list = list->next) {
                tdata = (TargetData *) list->data;
                targets[n_targets++] = tdata->target;
        }

        XChangeProperty (manager->mDisplay, rdata->requestor, rdata->property, XA_ATOM, 32, PropModeReplace, (unsigned char *) targets, n_targets);
        free (targets);
    } else  {
        /* Convert from stored CLIPBOARD data */
        list = list_find (manager->mContents, (ListFindFunc) find_content_target, (void *) rdata->target);

        /* We got a target that we don't support */
        if (!list) return;

        tdata = (TargetData *)list->data;
        if (tdata->type == XA_INCR) {
            /* we haven't completely received this target yet  */
            rdata->property = None;
            return;
        }

        rdata->data = target_data_ref (tdata);
        items = tdata->length / clipboard_bytes_per_item (tdata->format);
        if (tdata->length <= SELECTION_MAX_SIZE)
            XChangeProperty (manager->mDisplay, rdata->requestor, rdata->property, tdata->type, tdata->format, PropModeReplace, tdata->data, items);
        else {
            /* start incremental transfer */
            rdata->offset = 0;

            gdk_error_trap_push ();

            XGetWindowAttributes (manager->mDisplay, rdata->requestor, &atts);
            XSelectInput (manager->mDisplay, rdata->requestor, atts.your_event_mask | PropertyChangeMask);

            XChangeProperty (manager->mDisplay, rdata->requestor, rdata->property,
                             XA_INCR, 32, PropModeReplace, (unsigned char *) &items, 1);
            XSync (manager->mDisplay, False);
            gdk_error_trap_pop_ignored ();
        }
    }
}

void collect_incremental (IncrConversion* rdata, ClipboardManager* manager)
{
    if (rdata->offset >= 0)
        manager->mConversions = list_prepend (manager->mConversions, rdata);
    else {
        if (rdata->data) {
            target_data_unref (rdata->data);
            rdata->data = NULL;
        }
        free (rdata);
    }
}

TargetData* target_data_ref (TargetData *data)
{
    data->refcount++;
    return data;
}
