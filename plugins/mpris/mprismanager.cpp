/*

#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
*/

#include "mprismanager.h"
#include "clib-syslog.h"
#include "clib-syslog.h"

#define MPRIS_OBJECT_PATH  "/org/mpris/MediaPlayer2"
#define MPRIS_INTERFACE    "org.mpris.MediaPlayer2.Player"
#define MPRIS_PREFIX       "org.mpris.MediaPlayer2."

MprisManager* MprisManager::mMprisManager = nullptr;

/* Number of media players supported.
 * Correlates to the number of elements in BUS_NAMES */
static const int NUM_BUS_NAMES = 16;

/* Names to we want to watch */
static const char *BUS_NAMES[] = {"org.mpris.MediaPlayer2.audacious",
                                   "org.mpris.MediaPlayer2.clementine",
                                   "org.mpris.MediaPlayer2.vlc",
                                   "org.mpris.MediaPlayer2.mpd",
                                   "org.mpris.MediaPlayer2.exaile",
                                   "org.mpris.MediaPlayer2.banshee",
                                   "org.mpris.MediaPlayer2.rhythmbox",
                                   "org.mpris.MediaPlayer2.pragha",
                                   "org.mpris.MediaPlayer2.quodlibet",
                                   "org.mpris.MediaPlayer2.guayadeque",
                                   "org.mpris.MediaPlayer2.amarok",
                                   "org.mpris.MediaPlayer2.nuvolaplayer",
                                   "org.mpris.MediaPlayer2.xbmc",
                                   "org.mpris.MediaPlayer2.xnoise",
                                   "org.mpris.MediaPlayer2.gmusicbrowser",
                                   "org.mpris.MediaPlayer2.spotify"};


enum {
        PROP_0,
};
/* Returns the name of the media player.
 * User must free. */
/*gchar* get_player_name(const gchar *name)
{
    gchar **tokens;
    gchar *player_name;

    // max_tokens is 4 because a player could have additional instances,
    // like org.mpris.MediaPlayer2.vlc.instance7389
    tokens = g_strsplit (name, ".", 4);
    player_name = g_strdup (tokens[3]);
    g_strfreev (tokens);

    return player_name;
}*/
MprisManager::MprisManager()
{

}

MprisManager::~MprisManager()
{

}

QString& get_player_name(const char* name)
{
    QString ori_name(name);
    QString ret_name=ori_name.section(".",3,4);
    return ret_name;
}

/* A media player was just run and should be
 * added to the head of media_player_queue. */
void
MprisManager::mp_name_appeared (GDBusConnection  *connection,
                              const char      *name,
                              const char      *name_owner)
{
    QString player_name;
    CT_SYSLOG (LOG_DEBUG,"MPRIS Name acquired: %s\n", name);

    player_name=get_player_name(name);
    mMprisManager->media_player_queue->push_front(name);
}

/* A media player quit running and should be
 * removed from media_player_queue. */
void
MprisManager::mp_name_vanished (GDBusConnection *connection,
                                const char     *name)
{
    if(mMprisManager->media_player_queue->isEmpty())
        return;
    CT_SYSLOG (LOG_DEBUG,"MPRIS Name vanished: %s\n", name);

    QString player_name=get_player_name(name);
    if(mMprisManager->media_player_queue->contains(player_name))
        mMprisManager->media_player_queue->removeOne(player_name);
}

/* Code copied from Totem media player
 * src/plugins/media-player-keys/totem-media-player-keys.c */
void
MprisManager::on_media_player_key_pressed (const char *key)
{
    if(mMprisManager->media_player_queue->isEmpty())
        return;

    GDBusProxyFlags flags = G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START;
    GDBusProxy *proxy = NULL;
    GError *error = NULL;
    const char *mpris_key = NULL;
    QString mpris_head;
    QString mpris_name;

    if (strcmp ("Play", key) == 0)
        mpris_key = "PlayPause";
    else if (strcmp ("Pause", key) == 0)
        mpris_key = "Pause";
    else if (strcmp ("Previous", key) == 0)
        mpris_key = "Previous";
    else if (strcmp ("Next", key) == 0)
        mpris_key = "Next";
    else if (strcmp ("Stop", key) == 0)
        mpris_key = "Stop";

    if (mpris_key != NULL)
    {
        mpris_head=mMprisManager->media_player_queue->head();
        mpris_name=QString(MPRIS_PREFIX "%1").arg(mpris_head);
        CT_SYSLOG (LOG_DEBUG,"MPRIS Sending '%s' to '%s'!", mpris_key, mpris_head.toLatin1().data());

        proxy = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION,
                                              flags,
                                              NULL,
                                              mpris_name.toLatin1().data(),
                                              MPRIS_OBJECT_PATH,
                                              MPRIS_INTERFACE,
                                              NULL,
                                              &error);

        if (proxy == NULL){
            CT_SYSLOG(LOG_DEBUG,"Error creating proxy: %s\n", error->message);
            g_error_free(error);
            return;
        }

        g_dbus_proxy_call (proxy, mpris_key, NULL,G_DBUS_CALL_FLAGS_NONE,
                           -1, NULL, NULL, NULL);
        g_object_unref (proxy);

    }
}

void
MprisManager::grab_media_player_keys_cb (GDBusProxy       *proxy,
                           GAsyncResult     *res)
{
    GVariant *variant;
    GError *error = NULL;

    variant = g_dbus_proxy_call_finish (proxy, res, &error);

    if (NULL == variant) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            CT_SYSLOG(LOG_WARNING,"Failed to call \"GrabMediaPlayerKeys\": %s", error->message);
        g_error_free (error);
        return;
    }
    g_variant_unref (variant);
}

void MprisManager::grab_media_player_keys ()
{
    if (NULL == mMprisManager->media_keys_proxy)
        return;

    g_dbus_proxy_call (mMprisManager->media_keys_proxy,
                      "GrabMediaPlayerKeys",
                      g_variant_new ("(su)", "UsdMpris", 0),
                      G_DBUS_CALL_FLAGS_NONE,
                      -1, NULL,
                      (GAsyncReadyCallback) grab_media_player_keys_cb,
                      mMprisManager);
}

void
MprisManager::key_pressed (GDBusProxy          *proxy,
                          char               *sender_name,
                          char               *signal_name,
                          GVariant            *parameters)
{
    char *app, *cmd;

    if (strcmp (signal_name, "MediaPlayerKeyPressed") != 0)
        return;
    g_variant_get (parameters, "(ss)", &app, &cmd);
    if (strcmp (app, "UsdMpris") == 0) {
        on_media_player_key_pressed (cmd);
    }
    g_free (app);
    g_free (cmd);
}

void
MprisManager::got_proxy_cb (GObject           *source_object,
                            GAsyncResult      *res)
{
    GError *error = NULL;

    mMprisManager->media_keys_proxy = g_dbus_proxy_new_for_bus_finish (res, &error);

    if (NULL == mMprisManager->media_keys_proxy) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning ("Failed to contact settings daemon: %s", error->message);
        g_error_free (error);
        return;
    }

    grab_media_player_keys ();
    //TODO...
    g_signal_connect (G_OBJECT (mMprisManager->media_keys_proxy), "g-signal",
                      G_CALLBACK (key_pressed), mMprisManager);
}

void
MprisManager::usd_name_appeared (GDBusConnection     *connection,
                               const char         *name,
                               const char         *name_owner)
{
    GDBusProxyFlags flags=GDBusProxyFlags(G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES |
            G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START);
    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SESSION,
                              flags,
                              NULL,
                              "org.ukui.SettingsDaemon",
                              "/org/ukui/SettingsDaemon/MediaKeys",
                              "org.ukui.SettingsDaemon.MediaKeys",
                              NULL,
                              (GAsyncReadyCallback) got_proxy_cb,
                              mMprisManager);
}

void
MprisManager::usd_name_vanished (GDBusConnection   *connection,
                   const char       *name)
{
    if (NULL != mMprisManager->media_keys_proxy) {
        g_object_unref (mMprisManager->media_keys_proxy);
        mMprisManager->media_keys_proxy = NULL;
    }
}

bool MprisManager::MprisManagerStart (GError           **error)
{
    GBusNameWatcherFlags flags = G_BUS_NAME_WATCHER_FLAGS_NONE;
    int i;
    
    CT_SYSLOG (LOG_DEBUG,"Starting mpris manager");

    mMprisManager->media_player_queue=new QQueue<QString>();
    /* Register all the names we wish to watch.*/
    for (i = 0; i < NUM_BUS_NAMES; i++){
        g_bus_watch_name(G_BUS_TYPE_SESSION,
                         BUS_NAMES[i],
                         flags,
                         (GBusNameAppearedCallback) mp_name_appeared,
                         (GBusNameVanishedCallback) mp_name_vanished,
                         mMprisManager,
                         NULL);
    }

    watch_id = g_bus_watch_name (G_BUS_TYPE_SESSION,
                                "org.ukui.SettingsDaemon",
                                G_BUS_NAME_WATCHER_FLAGS_NONE,
                                (GBusNameAppearedCallback) usd_name_appeared,
                                (GBusNameVanishedCallback) usd_name_vanished,
                                mMprisManager, NULL);
    return true;
}

void MprisManager::MprisManagerStop()
{
    CT_SYSLOG (LOG_DEBUG,"Stopping mpris manager");

    if (media_keys_proxy != NULL) {
        g_object_unref (media_keys_proxy);
        media_keys_proxy = NULL;
    }

    if (watch_id != 0) {
        g_bus_unwatch_name (watch_id);
        watch_id = 0;
    }
}

MprisManager* MprisManager::MprisManagerNew()
{
    if(nullptr == mMprisManager)
        mMprisManager = new MprisManager();
    return mMprisManager;
}
