#include "background-manager.h"
#include "clib-syslog.h"

#include <glib.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gio/gio.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <X11/Xatom.h>
#include <glib-object.h>

#define UKUI_SESSION_MANAGER_DBUS_NAME "org.gnome.SessionManager"
#define UKUI_SESSION_MANAGER_DBUS_PATH "/org/gnome/SessionManager"

BackgroundManager* BackgroundManager::mBackgroundManager = nullptr;

void disconnect_screen_signals (BackgroundManager* manager);
void remove_background (BackgroundManager* manager);
void on_session_manager_signal (GDBusProxy* proxy, const gchar* senderName, const gchar* signalName, GVariant* parameters, gpointer udata);
void free_fade (BackgroundManager* manager);
void free_bg_surface (BackgroundManager* manager);
void real_draw_bg (BackgroundManager* manager, GdkScreen* screen);
void free_scr_sizes (BackgroundManager* manager);
bool can_fade_bg (BackgroundManager* manager);
bool settings_change_event_idle_cb (BackgroundManager* manager);
bool peony_is_drawing_bg (BackgroundManager* manager);
bool peony_can_draw_bg (BackgroundManager* manager);
bool usd_can_draw_bg (BackgroundManager* manager);
void on_screen_size_changed (GdkScreen* screen, BackgroundManager* manager);
void draw_background (BackgroundManager* manager, bool mayFade);
bool settings_change_event_cb (GSettings* settings, gpointer keys, gint nKeys, BackgroundManager* manager);
void connect_screen_signals (BackgroundManager* manager);
void on_bg_transitioned (MateBG*, BackgroundManager* manager);
void on_bg_changed (MateBG* bg, BackgroundManager* manager);
void queue_timeout (BackgroundManager* manager);
void setup_background (BackgroundManager* manager);
bool queue_setup_background (BackgroundManager* manager);
void draw_bg_after_session_loads (BackgroundManager* manager);
void disconnect_session_manager_listener (BackgroundManager* manager);
void on_bg_handling_changed (GSettings* settings, const char* key, BackgroundManager* manager);

BackgroundManager* BackgroundManager::getInstance()
{
    if (nullptr == mBackgroundManager) {
        mBackgroundManager = new BackgroundManager(nullptr);
    }

    return mBackgroundManager;
}

bool BackgroundManager::managerStart()
{
    mSetting = g_settings_new(MATE_BG_SCHEMA);

    mUsdCanDraw = g_settings_get_boolean (mSetting, MATE_BG_KEY_DRAW_BACKGROUND);
    mPeonyCanDraw = g_settings_get_boolean (mSetting, MATE_BG_KEY_SHOW_DESKTOP);

    g_signal_connect (mSetting, "changed::" MATE_BG_KEY_DRAW_BACKGROUND,
                  G_CALLBACK (on_bg_handling_changed), this);
    g_signal_connect (mSetting, "changed::" MATE_BG_KEY_SHOW_DESKTOP,
                  G_CALLBACK (on_bg_handling_changed), this);

    if (mUsdCanDraw) {
        if (mPeonyCanDraw) {
            draw_bg_after_session_loads (this);
        } else {
            setup_background (this);
        }
    }

    return true;
}

void BackgroundManager::managerStop()
{
    if (this->mProxy) {
        disconnect_session_manager_listener (this);
        g_object_unref (mProxy);
    }
    if (0 != mTimeoutID) {
        g_source_remove (mTimeoutID);
        mTimeoutID = 0;
    }

    remove_background (this);
}

BackgroundManager::BackgroundManager(QObject *parent) : QObject(parent)
{
}

void BackgroundManager::onBgHandingChangedSlot(const QString &)
{

}

void BackgroundManager::onSessionManagerSignal(GDBusProxy *, const gchar *, const gchar *, GVariant *, gpointer)
{

}

void draw_bg_after_session_loads (BackgroundManager* manager)
{
    GError *error = NULL;
    GDBusProxyFlags flags;

    flags = (GDBusProxyFlags)(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START);

    manager->mProxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION, flags, NULL, UKUI_SESSION_MANAGER_DBUS_NAME,
                        UKUI_SESSION_MANAGER_DBUS_PATH, UKUI_SESSION_MANAGER_DBUS_NAME, NULL, &error);
    if (nullptr == manager->mProxy) {
        CT_SYSLOG (LOG_ERR, "Could not listen to session manager: %s", error->message);
        g_error_free (error);
        return;
    }

    manager->mProxySignalID = g_signal_connect (manager->mProxy, "g-signal", G_CALLBACK (on_session_manager_signal), manager);
}

void on_session_manager_signal (GDBusProxy* proxy, const gchar* senderName, const gchar  *signalName, GVariant* parameters, gpointer udata)
{
    BackgroundManager* manager = static_cast<BackgroundManager*>(udata);

    if (g_strcmp0 (signalName, "SessionRunning") == 0) {
        queue_timeout (manager);
        disconnect_session_manager_listener (manager);
    }
}

void disconnect_session_manager_listener (BackgroundManager* manager)
{
    if (manager->mProxy && manager->mProxySignalID) {
        g_signal_handler_disconnect (manager->mProxy, manager->mProxySignalID);
        manager->mProxySignalID = 0;
    }
}

void queue_timeout (BackgroundManager* manager)
{
    if (manager->mTimeoutID > 0)
        return;

    /* SessionRunning: now check if Peony is drawing background, and if not, set it.
     *
     * FIXME: We wait a few seconds after the session is up because Peony tells the
     * session manager that its ready before it sets the background.
     * https://bugzilla.gnome.org/show_bug.cgi?id=568588
     */
    manager->mTimeoutID = g_timeout_add_seconds (8, (GSourceFunc) queue_setup_background, manager);
}

bool queue_setup_background (BackgroundManager* manager)
{
    manager->mTimeoutID = 0;

    setup_background (manager);

    return false;
}

void setup_background (BackgroundManager* manager)
{
    g_return_if_fail (manager->mMateBG == NULL);

    manager->mMateBG = mate_bg_new();

    manager->mDrawInProgress = false;

    g_signal_connect(manager->mMateBG, "changed", G_CALLBACK (on_bg_changed), manager);

    g_signal_connect(manager->mMateBG, "transitioned", G_CALLBACK (on_bg_transitioned), manager);

    mate_bg_load_from_gsettings (manager->mMateBG, manager->mSetting);

    connect_screen_signals (manager);

    g_signal_connect (manager->mSetting, "change-event", G_CALLBACK (settings_change_event_cb), manager);
}

void on_bg_changed (MateBG*, BackgroundManager* manager)
{
    draw_background (manager, true);
}

void on_bg_transitioned (MateBG*, BackgroundManager* manager)
{
    draw_background (manager, true);
}

void connect_screen_signals (BackgroundManager* manager)
{
    GdkDisplay *display   = gdk_display_get_default();
    int         n_screens = gdk_display_get_n_screens (display);
    int         i;

    for (i = 0; i < n_screens; i++) {
        GdkScreen *screen = gdk_display_get_screen (display, i);

        g_signal_connect (screen, "size-changed", G_CALLBACK (on_screen_size_changed), manager);
        g_signal_connect (screen, "monitors-changed", G_CALLBACK (on_screen_size_changed), manager);
    }
}

bool settings_change_event_cb (GSettings* settings, gpointer keys, gint nKeys, BackgroundManager* manager)
{
    /* Complements on_bg_handling_changed() */
    manager->mUsdCanDraw = usd_can_draw_bg (manager);
    manager->mPeonyCanDraw = peony_can_draw_bg (manager);

    if (manager->mUsdCanDraw && manager->mMateBG != NULL && !peony_is_drawing_bg (manager))
    {
        /* Defer signal processing to avoid making the dconf backend deadlock */
        g_idle_add ((GSourceFunc) settings_change_event_idle_cb, manager);
    }

    return FALSE;   /* let the event propagate further */
}

void draw_background (BackgroundManager* manager, bool mayFade)
{
    if (!manager->mUsdCanDraw || manager->mDrawInProgress || peony_is_drawing_bg (manager))
        return;

    GdkDisplay *display   = gdk_display_get_default ();
    int         n_screens = gdk_display_get_n_screens (display);
    int         scr;

    manager->mDrawInProgress = false;
    manager->mDoFade = mayFade && can_fade_bg (manager);
    free_scr_sizes (manager);

    for (scr = 0; scr < n_screens; scr++)
    {
        CT_SYSLOG(LOG_ERR, "Drawing background on Screen%d", scr);
        real_draw_bg (manager, gdk_display_get_screen (display, scr));
    }
    manager->mScrSizes = g_list_reverse (manager->mScrSizes);

    manager->mDrawInProgress = false;
}

void on_screen_size_changed (GdkScreen* screen, BackgroundManager* manager)
{
    if (!manager->mUsdCanDraw || manager->mDrawInProgress || peony_is_drawing_bg (manager))
        return;

    gint scrNum = gdk_screen_get_number (screen);
    gchar* oldSize = (gchar*)g_list_nth_data (manager->mScrSizes, scrNum);
    gchar* newSize = g_strdup_printf ("%dx%d", gdk_screen_get_width (screen), gdk_screen_get_height (screen));
    if (g_strcmp0 (oldSize, newSize) != 0)
    {
        CT_SYSLOG(LOG_DEBUG, "Screen%d size changed: %s -> %s", scrNum, oldSize, newSize);
        draw_background (manager, FALSE);
    } else {
        CT_SYSLOG(LOG_DEBUG, "Screen%d size unchanged (%s). Ignoring.", scrNum, oldSize);
    }
    g_free (newSize);
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

    gdk_error_trap_push();

    XGetWindowProperty (display, peonyWindow, wmclassProp, 0, 20, False, XA_STRING, &type, &format, &nitems, &after, &data);

    XSync (display, false);

    if (gdk_error_trap_pop() == BadWindow || data == NULL)
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

bool settings_change_event_idle_cb (BackgroundManager* manager)
{
    mate_bg_load_from_gsettings (manager->mMateBG, manager->mSetting);

    return false;   /* remove from the list of event sources */
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

void real_draw_bg (BackgroundManager* manager, GdkScreen* screen)
{
    GdkWindow *window = gdk_screen_get_root_window (screen);
    gint width   = gdk_screen_get_width (screen);
    gint height  = gdk_screen_get_height (screen);

    free_bg_surface (manager);
    manager->mSurface = mate_bg_create_surface (manager->mMateBG, window, width, height, false);

    if (manager->mDoFade)
    {
        free_fade (manager);
        manager->mFade = mate_bg_set_surface_as_root_with_crossfade (screen, manager->mSurface);
        g_signal_connect_swapped (manager->mFade, "finished", G_CALLBACK (free_fade), manager);
    }
    else
    {
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

void free_fade (BackgroundManager* manager)
{
    if (nullptr != manager->mFade) {
        g_object_unref (manager->mFade);
        manager->mFade = nullptr;
    }
}

void remove_background (BackgroundManager* manager)
{
    disconnect_screen_signals (manager);

    g_signal_handlers_disconnect_by_func (manager->mSetting, (gpointer)settings_change_event_cb, manager);

    if (nullptr != manager->mSetting) {
        g_object_unref (G_OBJECT (manager->mSetting));
        manager->mSetting = nullptr;
    }

    if (nullptr != manager->mMateBG) {
        g_object_unref (G_OBJECT (manager->mMateBG));
        manager->mMateBG = nullptr;
    }

    free_scr_sizes (manager);
    free_bg_surface (manager);
    free_fade (manager);
}

void disconnect_screen_signals (BackgroundManager* manager)
{
    int                 i;
    GdkDisplay          *display  = gdk_display_get_default();
    int                 n_screens = gdk_display_get_n_screens (display);

    for (i = 0; i < n_screens; i++) {
        g_signal_handlers_disconnect_by_func (gdk_display_get_screen (display, i), (gpointer)G_CALLBACK (on_screen_size_changed), manager);
    }
}

void on_bg_handling_changed (GSettings* settings, const char* key, BackgroundManager* manager)
{
    if (peony_is_drawing_bg (manager)) {
        if (nullptr != manager->mMateBG) remove_background (manager);
    } else if (manager->mUsdCanDraw && nullptr == manager->mMateBG) {
        setup_background (manager);
    }
}
