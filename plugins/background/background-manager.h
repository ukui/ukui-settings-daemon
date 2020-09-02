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
#ifndef BACKGROUND_MANAGER_H
#define BACKGROUND_MANAGER_H
#include <QObject>
#include <QDBusServiceWatcher>
#include <QDBusInterface>
#include <QDBusConnection>
#include <QTimer>


#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-bg.h>

class BackgroundManager : public QObject
{
    Q_OBJECT
public:
    static BackgroundManager* getInstance();
    ~BackgroundManager();
    bool managerStart();
    void managerStop();
    int mCallCount;

public:
    void draw_bg_after_session_loads ();
    void disconnect_session_manager_listener ();
    static void setup_background (BackgroundManager *manager);
    void on_screen_size_changed (GdkScreen* screen, BackgroundManager* manager);
    static void screen_change (BackgroundManager *manager);
    static bool settings_change_event_cb (GSettings* settings, gpointer keys, gint nKeys, BackgroundManager* manager);
    static void onBgHandingChangedSlot (GSettings* settings, const char* key, BackgroundManager* manager);
    void remove_background ();


private Q_SLOTS:
    void onSessionManagerSignal(QString, bool);
    void SettingsChangeEventIdleCb ();
    void callBackDrow();
private:
    BackgroundManager()=delete;
    BackgroundManager(BackgroundManager&) = delete;
    BackgroundManager& operator= (const BackgroundManager&) = delete;
    BackgroundManager(QObject *parent = nullptr);

    friend void free_fade (BackgroundManager* manager);
    friend void free_bg_surface (BackgroundManager* manager);
    friend void real_draw_bg (BackgroundManager* manager, GdkScreen* screen);
    friend void free_scr_sizes (BackgroundManager* manager);
    friend bool can_fade_bg (BackgroundManager* manager);
    friend bool peony_is_drawing_bg (BackgroundManager* manager);
    friend bool peony_can_draw_bg (BackgroundManager* manager);
    friend bool usd_can_draw_bg (BackgroundManager* manager);
//    friend void on_screen_size_changed (GdkScreen* screen, BackgroundManager* manager);
    friend void draw_background (BackgroundManager* manager, bool mayFade);
    friend void queue_timeout (BackgroundManager* manager);
    friend bool queue_setup_background (BackgroundManager* manager);

private:
    GSettings              *mSetting;
    QTimer                 *mTime;
    MateBG                 *mMateBG;
    cairo_surface_t        *mSurface;
    MateBGCrossfade        *mFade;
    GList                  *mScrSizes;

    bool                    mUsdCanDraw;
    bool                    mPeonyCanDraw;
    bool                    mDoFade;
    bool                    mDrawInProgress;
    QDBusInterface         *mDbusInterface;

    static BackgroundManager*   mBackgroundManager;
};

#endif // BACKGROUND_MANAGER_H
