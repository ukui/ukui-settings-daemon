/*
 */

#ifndef MPRISMANAGER_H
#define MPRISMANAGER_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>        //for GDBusProxy
#include <QQueue>

class MprisManager{
public:
    ~MprisManager();
    static MprisManager* MprisManagerNew();
    bool MprisManagerStart(GError **error);
    void MprisManagerStop();

private:
    MprisManager();
    static void mp_name_appeared(GDBusConnection  *connection,
                          const char      *name,
                          const char      *name_owner);
    static void mp_name_vanished (GDBusConnection *connection,
                                  const char     *name);
    static void on_media_player_key_pressed (const char      *key);
    static void grab_media_player_keys_cb (GDBusProxy       *proxy,
                                    GAsyncResult     *res);
    static void grab_media_player_keys();
    static void key_pressed (GDBusProxy          *proxy,
                             char               *sender_name,
                             char               *signal_name,
                             GVariant            *parameters);
    static void got_proxy_cb (GObject           *source_object,
                                GAsyncResult      *res);
    static void usd_name_appeared (GDBusConnection     *connection,
                                    const char         *name,
                                    const char         *name_owner);
    static void usd_name_vanished (GDBusConnection   *connection,
                                    const char       *name);

private:
    static MprisManager   *mMprisManager;
    QQueue<QString>       *media_player_queue;
    GDBusProxy            *media_keys_proxy;
    guint                 watch_id;
};

#endif /* MPRISMANAGER_H */
