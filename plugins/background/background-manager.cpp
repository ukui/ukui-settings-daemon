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
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QStringList>
#include <QDBusReply>
#include <QDBusMessage>
#include <QObject>
#include "background-manager.h"
#include "clib-syslog.h"
#include <glib.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <X11/Xatom.h>
#include <QtX11Extras/QX11Info>
#define UKUI_SESSION_MANAGER_DBUS_NAME "org.gnome.SessionManager"
#define UKUI_SESSION_MANAGER_DBUS_PATH "/org/gnome/SessionManager"

BackgroundManager* BackgroundManager::mBackgroundManager = nullptr;

BackgroundManager::BackgroundManager(QObject *parent) : QObject(parent)
{
    mSetting = nullptr;
    mMateBG  = nullptr;
    mSurface = nullptr;
    mFade    = nullptr;
    mScrSizes= nullptr;
    mDbusInterface = nullptr;
    mTime   = new QTimer(this);
    mCallCount = 0;
}

BackgroundManager::~BackgroundManager()
{
    if(mTime)
        delete mTime;
}

BackgroundManager* BackgroundManager::getInstance()
{
    if (nullptr == mBackgroundManager) {
        mBackgroundManager = new BackgroundManager(nullptr);
    }
    return mBackgroundManager;
}

void BackgroundManager::onSessionManagerSignal(QString signalName,bool res)
{
    if (signalName == "SessionRunning") {
        queue_timeout (this);
        disconnect_session_manager_listener ();
    }
}

void BackgroundManager::draw_bg_after_session_loads ()
{
    GDBusProxyFlags flags;
    QDBusConnection conn = QDBusConnection::sessionBus();


    flags = (GDBusProxyFlags)(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START);

    mDbusInterface = new QDBusInterface(UKUI_SESSION_MANAGER_DBUS_NAME,
                                        UKUI_SESSION_MANAGER_DBUS_PATH,
                                        UKUI_SESSION_MANAGER_DBUS_NAME,
                                        conn);
    if (!mDbusInterface->isValid()) {
        qDebug ("Could not listen to session manager");
        return;
    }

    connect(mDbusInterface,SIGNAL(moduleStateChanged(QString,bool)),this,SLOT(onSessionManagerSignal(QString,bool)));
}

void BackgroundManager::disconnect_session_manager_listener ()
{
    if (mDbusInterface) {
        delete mDbusInterface;
    }
}

void queue_timeout (BackgroundManager* manager)
{
    manager->setup_background (manager);
}

void BackgroundManager::on_screen_size_changed (GdkScreen* screen, BackgroundManager* manager)
{
    if (!manager->mUsdCanDraw || manager->mDrawInProgress || peony_is_drawing_bg (manager))
        return;
    GdkWindow   *window;
    Screen      *xscreen;
    gchar       *oldSize;
    gchar       *newSize;
    int   scale, scr_num;

    window = gdk_screen_get_root_window (screen);
    scale = gdk_window_get_scale_factor (window);
    xscreen = gdk_x11_screen_get_xscreen (screen);
    scr_num = gdk_x11_screen_get_screen_number (screen);
    oldSize = (gchar*)g_list_nth_data (manager->mScrSizes, scr_num);
    newSize = g_strdup_printf ("%dx%d", WidthOfScreen (xscreen) / scale, HeightOfScreen (xscreen) / scale);
    qDebug("oldSize = %s, newSize=%s, scale=%d",oldSize, newSize, scale);
    if (g_strcmp0 (oldSize, newSize) != 0)
    {
        qDebug("Screen size changed: %s -> %s", oldSize, newSize);
        draw_background (manager, false);
    } else {
        qDebug("Screen size unchanged (%s). Ignoring.", oldSize);
    }
    g_free (newSize);
}

void connect_screen_signals (BackgroundManager* manager)
{
    GdkScreen *screen = gdk_screen_get_default();
    QDBusConnection::sessionBus().connect(QString(),
                                          QString("/backend"),
                                          "org.kde.kscreen.Backend","configChanged",
                                          manager,
                                          SLOT(callBackDrow()));
//    g_signal_connect (screen, "size-changed", G_CALLBACK (on_screen_size_changed), manager););
}
void BackgroundManager::callBackDrow(){
    mCallCount++;
    if(mCallCount == 2){
        GdkScreen *qscreen = gdk_screen_get_default();
        QScreen *screenx = QApplication::primaryScreen();
        QRect rect =screenx->availableGeometry() ;
        qDebug()<<"width:"<<rect.width()<<"height:"<<rect.height();
        on_screen_size_changed(qscreen,this);
        mCallCount = 0;
    }
}

void on_bg_changed (MateBG*, BackgroundManager* manager)
{
    draw_background (manager, true);
}

void on_bg_transitioned (MateBG*, BackgroundManager* manager)
{
    draw_background (manager, true);
}

void BackgroundManager::SettingsChangeEventIdleCb ()
{
    mTime->stop();
    mate_bg_load_from_gsettings (mMateBG, mSetting);
}

bool BackgroundManager::settings_change_event_cb (GSettings* settings, gpointer keys, gint nKeys, BackgroundManager* manager)
{
    /* Complements on_bg_handling_changed() */
    manager->mUsdCanDraw = usd_can_draw_bg (manager);
    manager->mPeonyCanDraw = peony_can_draw_bg (manager);

    if (manager->mUsdCanDraw && manager->mMateBG != NULL && !peony_is_drawing_bg (manager))
    {
        /* Defer signal processing to avoid making the dconf backend deadlock */
        connect(manager->mTime,SIGNAL(timeout()),manager,SLOT(SettingsChangeEventIdleCb()));
        manager->mTime->start();
    }

    return false;   /* let the event propagate further */
}

void BackgroundManager::setup_background (BackgroundManager *manager)
{
    GdkScreen *screen = gdk_screen_get_default();

    if(!manager->mMateBG == NULL)
        return;

    manager->mMateBG = mate_bg_new();

    manager->mDrawInProgress = true;

    g_signal_connect(manager->mMateBG, "changed", G_CALLBACK (on_bg_changed), manager);

    g_signal_connect(manager->mMateBG, "transitioned", G_CALLBACK (on_bg_transitioned), manager);

    mate_bg_load_from_gsettings (manager->mMateBG, manager->mSetting);
    connect_screen_signals (manager);
    g_signal_connect (manager->mSetting, "change-event",
                      G_CALLBACK (settings_change_event_cb), manager);
}

void draw_background (BackgroundManager* manager, bool mayFade)
{

//    if (!manager->mUsdCanDraw || manager->mDrawInProgress || peony_is_drawing_bg (manager))
//        return;
    manager->mDrawInProgress = true;
    manager->mDoFade = mayFade && can_fade_bg (manager);
    free_scr_sizes (manager);

    real_draw_bg (manager, gdk_screen_get_default());
    manager->mScrSizes = g_list_reverse (manager->mScrSizes);

    manager->mDrawInProgress = false;
}

bool usd_can_draw_bg (BackgroundManager* manager)
{
    return g_settings_get_boolean (manager->mSetting, MATE_BG_KEY_DRAW_BACKGROUND) == TRUE?true:false;
}

bool peony_can_draw_bg (BackgroundManager* manager)
{
    return g_settings_get_boolean (manager->mSetting, MATE_BG_KEY_SHOW_DESKTOP) == TRUE?true:false;
}

bool peony_is_drawing_bg (BackgroundManager* manager)
{
    int                 format;
    bool                running = false;
    unsigned long       nitems, after;
    unsigned char*      data;
    Window              peonyWindow;
    Atom                peonyProp, wmclassProp, type;
    Display*            display = gdk_x11_get_default_xdisplay ();
    Window              window = gdk_x11_get_default_root_xwindow ();

    if (!manager->mPeonyCanDraw)
        return false;

    peonyProp = XInternAtom (display, "PEONY_DESKTOP_WINDOW_ID", True);
    if (peonyProp == None)
        return false;

    XGetWindowProperty (display, window, peonyProp, 0, 1, False, XA_WINDOW, &type, &format, &nitems, &after, &data);
    if (data == NULL)
        return false;

    peonyWindow = *(Window *) data;
    XFree (data);

    if (type != XA_WINDOW || format != 32)
        return false;

    wmclassProp = XInternAtom (display, "WM_CLASS", true);
    if (wmclassProp == None)
        return false;

    gdk_x11_display_error_trap_push(gdk_display_get_default());
    XGetWindowProperty (display, peonyWindow, wmclassProp, 0, 20, False,
                        XA_STRING, &type, &format, &nitems, &after, &data);

    XSync (display, False);

    if (gdk_x11_display_error_trap_pop(gdk_display_get_default()) == BadWindow || data == NULL)
        return false;

    /* See: peony_desktop_window_new(), in src/peony-desktop-window.c */
    if (nitems == 21 && after == 0 && format == 8 &&
            !strcmp((char*) data, "desktop_window") &&
            !strcmp((char*) data + strlen((char*) data) + 1, "Peony")) {
        running = true;
    }
    XFree (data);

    return running;
}

bool can_fade_bg (BackgroundManager* manager)
{
    return g_settings_get_boolean (manager->mSetting, MATE_BG_KEY_BACKGROUND_FADE);
}

void free_scr_sizes (BackgroundManager* manager)
{
    if (manager->mScrSizes != NULL) {
        g_list_foreach (manager->mScrSizes, (GFunc)g_free, NULL);
        g_list_free (manager->mScrSizes);
        manager->mScrSizes = nullptr;
    }
}

void free_fade (BackgroundManager* manager)
{
    if (nullptr != manager->mFade) {
        g_object_unref (manager->mFade);
        manager->mFade = nullptr;
    }
}

void real_draw_bg (BackgroundManager* manager, GdkScreen* screen)
{
    GdkWindow *window = gdk_screen_get_root_window (screen);
    int scale   = gdk_window_get_scale_factor (window);
    int width   = WidthOfScreen (gdk_x11_screen_get_xscreen (screen)) / scale;
    int height  = HeightOfScreen (gdk_x11_screen_get_xscreen (screen)) / scale;
    free_bg_surface (manager);
    qDebug("width = %d, height=%d, scale=%d",width, height, scale);
    manager->mSurface = mate_bg_create_surface_scale (manager->mMateBG, window, width,
                                                      height, scale, TRUE);

    if (manager->mDoFade) {
        free_fade (manager);
        manager->mFade = mate_bg_set_surface_as_root_with_crossfade (screen, manager->mSurface);
        g_signal_connect_swapped (manager->mFade, "finished", G_CALLBACK (free_fade), manager);
    } else {
        mate_bg_set_surface_as_root (screen, manager->mSurface);
    }
    manager->mScrSizes = g_list_prepend (manager->mScrSizes, g_strdup_printf ("%dx%d", width, height));
}

void free_bg_surface (BackgroundManager* manager)
{
    if (nullptr != manager->mSurface) {
        cairo_surface_destroy (manager->mSurface);
        manager->mSurface = nullptr;
    }
}

void disconnect_screen_signals (BackgroundManager* manager)
{
//    on_screen_size_changed(gdk_screen_get_default(), manager);
}

void BackgroundManager::remove_background ()
{
    disconnect_screen_signals (this);
    g_signal_handlers_disconnect_by_func (mSetting, (gpointer)settings_change_event_cb, this);

    if (nullptr != mSetting) {
        g_object_unref (G_OBJECT (mSetting));
        mSetting = nullptr;
    }

    if (nullptr != mMateBG) {
        g_object_unref (G_OBJECT (mMateBG));
        mMateBG = nullptr;
    }

    free_scr_sizes (this);
    free_bg_surface (this);
    free_fade (this);
}

void BackgroundManager::onBgHandingChangedSlot (GSettings* settings, const char* key, BackgroundManager* manager)
{
    if (peony_is_drawing_bg (manager)) {
        if (nullptr != manager->mMateBG)
            manager->remove_background();
    } else if (manager->mUsdCanDraw && nullptr == manager->mMateBG) {
        setup_background (manager);
    }
}


bool BackgroundManager::managerStart()
{
    mSetting = g_settings_new(MATE_BG_SCHEMA);

    mUsdCanDraw = g_settings_get_boolean (mSetting, MATE_BG_KEY_DRAW_BACKGROUND);
    mPeonyCanDraw = g_settings_get_boolean (mSetting, MATE_BG_KEY_SHOW_DESKTOP);

    g_signal_connect (mSetting, "changed::" MATE_BG_KEY_DRAW_BACKGROUND,
                  G_CALLBACK (onBgHandingChangedSlot), this);
    g_signal_connect (mSetting, "changed::" MATE_BG_KEY_SHOW_DESKTOP,
                  G_CALLBACK (onBgHandingChangedSlot), this);
    g_signal_connect (mSetting, "changed::" MATE_BG_KEY_PRIMARY_COLOR,
                  G_CALLBACK (onBgHandingChangedSlot), this);
    g_signal_connect (mSetting, "changed::" MATE_BG_KEY_PICTURE_FILENAME,
                  G_CALLBACK (onBgHandingChangedSlot), this);

    if (mUsdCanDraw) {
        if (mPeonyCanDraw) {
            draw_bg_after_session_loads ();
        } else {
            setup_background (this);
        }
    }
    return true;
}

void BackgroundManager::managerStop()
{
    remove_background ();
}
