#include "backgroundmanager.h"
#include "clib_syslog.h"

#define UKUI_SESSION_MANAGER_DBUS_NAME "org.gnome.SessionManager"
#define UKUI_SESSION_MANAGER_DBUS_PATH "/org/gnome/SessionManager"

BackgroundManager* BackgroundManager::mBackgroundManager = nullptr;

BackgroundManager::~BackgroundManager()
{
    delete mBackgroundManager;
}

BackgroundManager& BackgroundManager::backgroundManagerNew()
{
    if (nullptr == mBackgroundManager) {
        mBackgroundManager = new BackgroundManager();
    }

    return *mBackgroundManager;
}

bool BackgroundManager::backgroundManagerStart()
{
    CT_SYSLOG(LOG_DEBUG, "Starting background manager!");

//    mSettings = g_settings_new(MATE_BG_SCHEMA);

//    mUsdCanDraw = g_settings_get_boolean (mSettings, MATE_BG_KEY_DRAW_BACKGROUND);
//    mPeonyCanDraw = g_settings_get_boolean (mSettings, MATE_BG_KEY_SHOW_DESKTOP);

//    // FIXME://
//    g_signal_connect (mSettings, "changed::" MATE_BG_KEY_DRAW_BACKGROUND, G_CALLBACK (on_bg_handling_changed), manager);
//    g_signal_connect (mSettings, "changed::" MATE_BG_KEY_SHOW_DESKTOP, G_CALLBACK (on_bg_handling_changed), manager);




}

void BackgroundManager::backgroundManagerStop()
{
    CT_SYSLOG (LOG_DEBUG, "Stopping background manager");
}

BackgroundManager::BackgroundManager()
{

}
