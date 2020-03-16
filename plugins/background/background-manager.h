#ifndef BACKGROUND_MANAGER_H
#define BACKGROUND_MANAGER_H

#include <gobject/gvaluecollector.h>
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-bg.h>

#include <QObject>

class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    static BackgroundManager* getInstance();

    bool managerStart();
    void managerStop();

private:
    BackgroundManager()=delete;
    BackgroundManager(BackgroundManager&) = delete;
    BackgroundManager& operator= (const BackgroundManager&) = delete;

    BackgroundManager(QObject *parent = nullptr);

    friend void remove_background (BackgroundManager* manager);
    friend void free_fade (BackgroundManager* manager);
    friend void free_bg_surface (BackgroundManager* manager);
    friend void real_draw_bg (BackgroundManager* manager, GdkScreen* screen);
    friend void free_scr_sizes (BackgroundManager* manager);
    friend bool can_fade_bg (BackgroundManager* manager);
    friend bool settings_change_event_idle_cb (BackgroundManager* manager);
    friend bool peony_is_drawing_bg (BackgroundManager* manager);
    friend bool peony_can_draw_bg (BackgroundManager* manager);
    friend bool usd_can_draw_bg (BackgroundManager* manager);
    friend void on_screen_size_changed (GdkScreen* screen, BackgroundManager* manager);
    friend void draw_background (BackgroundManager* manager, bool mayFade);
    friend bool settings_change_event_cb (GSettings* settings, gpointer keys, gint nKeys, BackgroundManager* manager);
    friend void queue_timeout (BackgroundManager* manager);
    friend void setup_background (BackgroundManager* manager);
    friend bool queue_setup_background (BackgroundManager* manager);
    friend void draw_bg_after_session_loads (BackgroundManager* manager);
    friend void disconnect_session_manager_listener (BackgroundManager* manager);

Q_SLOT void onBgHandingChangedSlot(const QString&);
Q_SLOT void onSessionManagerSignal(GDBusProxy*, const gchar*, const gchar*, GVariant*, gpointer);


private:
    GSettings*                  mSetting;
    MateBG*                     mMateBG;
    cairo_surface_t*            mSurface;
    MateBGCrossfade*            mFade;
    GList*                      mScrSizes;

    bool                        mUsdCanDraw;
    bool                        mPeonyCanDraw;
    bool                        mDoFade;
    bool                        mDrawInProgress;
    guint                       mTimeoutID;

    GDBusProxy*                 mProxy;
    guint                       mProxySignalID;

    static BackgroundManager*   mBackgroundManager;
};

#endif // BACKGROUND_MANAGER_H
