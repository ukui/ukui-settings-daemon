/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2020 KylinSoft Co., Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef CLIPBOARDMANAGER_H
#define CLIPBOARDMANAGER_H
#include <QThread>

#include "list.h"

#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* always defined to indicate that i18n is enabled */
#define ENABLE_NLS 1

/* enable profiling */
/* #undef ENABLE_PROFILING */

/* Name of default gettext domain */
#define GETTEXT_PACKAGE "ukui-settings-daemon"

/* Warn on use of APIs added after GLib 2.36 */
#define GLIB_VERSION_MAX_ALLOWED GLIB_VERSION_2_36

/* Warn on use of APIs deprecated before GLib 2.36 */
#define GLIB_VERSION_MIN_REQUIRED GLIB_VERSION_2_36

/* Define to 1 if you have the `bind_textdomain_codeset' function. */
#define HAVE_BIND_TEXTDOMAIN_CODESET 1

/* Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define to 1 if you have the `dcgettext' function. */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the <dlfcn.h> header file. */
#define HAVE_DLFCN_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define if your <locale.h> file defines LC_MESSAGES. */
#define HAVE_LC_MESSAGES 1

/* Define if libcanberra-gtk3 is available */
#define HAVE_LIBCANBERRA 1

/* Define if libmatemixer is available */
#define HAVE_LIBMATEMIXER 1

/* Define if libnotify is available */
#define HAVE_LIBNOTIFY 1

/* Define to 1 if you have the <locale.h> header file. */
#define HAVE_LOCALE_H 1

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Defined if PolicyKit support is enabled */
#define HAVE_POLKIT 1

/* Define if PulseAudio support is available */
/* #undef HAVE_PULSE */

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <X11/extensions/xf86misc.h> header file. */
/* #undef HAVE_X11_EXTENSIONS_XF86MISC_H */

/* Define to 1 if you have the <X11/extensions/XKB.h> header file. */
#define HAVE_X11_EXTENSIONS_XKB_H 1

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

class ClipboardManager : public QThread
{
    Q_OBJECT

public:
    explicit ClipboardManager(QObject *parent = nullptr);
    ~ClipboardManager();

    bool managerStart ();
    bool managerStop ();
    void run() override;

private:
    bool                    mExit;
    Display*                mDisplay;
    Window                  mWindow;
    Time                    mTimestamp;

    List*                   mContents;
    List*                   mConversions;

    Window                  mRequestor;
    Atom                    mProperty;
    Time                    mTime;

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
