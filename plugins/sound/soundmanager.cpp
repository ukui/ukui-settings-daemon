//#ifdef HAVE_PULSE
#include <pulse/pulseaudio.h>
//#endif
#include <stdlib.h>
#include "soundmanager.h"
#include "clib-syslog.h"

SoundManager* SoundManager::mSoundManager = nullptr;

SoundManager::SoundManager()
{

}
SoundManager::~SoundManager()
{
    CT_SYSLOG(LOG_DEBUG,"SoundManager destructor!");
    if(mSoundManager)
        delete mSoundManager;
}

//#ifdef HAVE_PULSE

void
sample_info_cb (pa_context *c, const pa_sample_info *i, int eol, void *userdata)
{
        pa_operation *o;
        if (!i)
                return;
        CT_SYSLOG(LOG_DEBUG,"Found sample %s", i->name);

        /* We only flush those samples which have an XDG sound name
         * attached, because only those originate from themeing  */
        if (!(pa_proplist_gets (i->proplist, PA_PROP_EVENT_ID)))
                return;

        CT_SYSLOG(LOG_DEBUG,"Dropping sample %s from cache", i->name);

        if (!(o = pa_context_remove_sample (c, i->name, NULL, NULL))) {
                CT_SYSLOG(LOG_DEBUG,"pa_context_remove_sample (): %s", pa_strerror (pa_context_errno (c)));
                return;
        }

        pa_operation_unref (o);

        /* We won't wait until the operation is actually executed to
         * speed things up a bit.*/
}

void flush_cache (void)
{
        pa_mainloop *ml = NULL;
        pa_context *c = NULL;
        pa_proplist *pl = NULL;
        pa_operation *o = NULL;
        CT_SYSLOG(LOG_DEBUG,"Flushing sample cache");

        if (!(ml = pa_mainloop_new ())) {
                CT_SYSLOG(LOG_DEBUG,"Failed to allocate pa_mainloop");
                goto fail;
        }

        if (!(pl = pa_proplist_new ())) {
                CT_SYSLOG(LOG_DEBUG,"Failed to allocate pa_proplist");
                goto fail;
        }

        pa_proplist_sets (pl, PA_PROP_APPLICATION_NAME, PACKAGE_NAME);
        pa_proplist_sets (pl, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);
        pa_proplist_sets (pl, PA_PROP_APPLICATION_ID, "org.ukui.SettingsDaemon");

        if (!(c = pa_context_new_with_proplist (pa_mainloop_get_api (ml), PACKAGE_NAME, pl))) {
                CT_SYSLOG(LOG_DEBUG,"Failed to allocate pa_context");
                goto fail;
        }

        pa_proplist_free (pl);
        pl = NULL;

        if (pa_context_connect (c, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
                CT_SYSLOG(LOG_DEBUG,"pa_context_connect(): %s", pa_strerror (pa_context_errno (c)));
                goto fail;
        }

        /* Wait until the connection is established */
        while (pa_context_get_state (c) != PA_CONTEXT_READY) {

                if (!PA_CONTEXT_IS_GOOD (pa_context_get_state (c))) {
                        CT_SYSLOG(LOG_DEBUG,"Connection failed: %s", pa_strerror (pa_context_errno (c)));
                        goto fail;
                }

                if (pa_mainloop_iterate (ml, TRUE, NULL) < 0) {
                        CT_SYSLOG(LOG_DEBUG,"pa_mainloop_iterate() failed");
                        goto fail;
                }
        }

        /* Enumerate all cached samples */
        if (!(o = pa_context_get_sample_info_list (c, sample_info_cb, NULL))) {
                CT_SYSLOG(LOG_DEBUG,"pa_context_get_sample_info_list(): %s", pa_strerror (pa_context_errno (c)));
                goto fail;
        }

        /* Wait until our operation is finished and there's nothing
         * more queued to send to the server */
        while (pa_operation_get_state (o) == PA_OPERATION_RUNNING || pa_context_is_pending (c)) {

                if (!PA_CONTEXT_IS_GOOD (pa_context_get_state (c))) {
                        CT_SYSLOG(LOG_DEBUG,"Connection failed: %s", pa_strerror (pa_context_errno (c)));
                        goto fail;
                }

                if (pa_mainloop_iterate (ml, TRUE, NULL) < 0) {
                        CT_SYSLOG(LOG_DEBUG,"pa_mainloop_iterate() failed");
                        goto fail;
                }
        }

        CT_SYSLOG(LOG_DEBUG,"Sample cache flushed");

fail:
        if (o) {
                pa_operation_cancel (o);
                pa_operation_unref (o);
        }
        if (c) {
                pa_context_disconnect (c);
                pa_context_unref (c);
        }
        if (pl)
                pa_proplist_free (pl);
        if (ml)
                pa_mainloop_free (ml);
}

bool SoundManager::flush_cb ()
{
        flush_cache ();
        mSoundManager->timeout = 0;
        return false;
}

void SoundManager::trigger_flush ()
{

        if (mSoundManager->timeout)
                g_source_remove (mSoundManager->timeout);

        /* We delay the flushing a bit so that we can coalesce
         * multiple changes into a single cache flush */
        mSoundManager->timeout = g_timeout_add (500, (GSourceFunc) flush_cb, mSoundManager);
}

void
SoundManager::gsettings_notify_cb (GSettings *client,
                                   char *key)
{
        mSoundManager->trigger_flush();
}

void
SoundManager::file_monitor_changed_cb (GFileMonitor *monitor,
                         GFile *file,
                         GFile *other_file,
                         GFileMonitorEvent event)
{
        CT_SYSLOG(LOG_DEBUG,"Theme dir changed");
        mSoundManager->trigger_flush ();
}

bool
SoundManager::register_directory_callback (const char *path,
                             GError **error)
{
        GFile *f;
        GFileMonitor *m;
        bool succ = false;

        CT_SYSLOG(LOG_DEBUG,"Registering directory monitor for %s", path);

        f = g_file_new_for_path (path);

        m = g_file_monitor_directory (f, (GFileMonitorFlags)0, NULL, error);

        if (m != NULL) {
                g_signal_connect (m, "changed", G_CALLBACK (file_monitor_changed_cb), mSoundManager);

                //mSoundManager->monitors = g_list_prepend (mSoundManager->monitors, m);
                mSoundManager->monitors2->push_front(m);

                succ = true;
        }

        g_object_unref (f);

        return succ;
}

//#endif

bool SoundManager::SoundManagerStart (GError **error)
{


//#ifdef HAVE_PULSE
        char *p, **ps, **k;
        const char *env, *dd;
//#endif

        CT_SYSLOG(LOG_DEBUG,"Starting sound manager");
        monitors2 = new QList<GFileMonitor*>();

//#ifdef HAVE_PULSE

        /* We listen for change of the selected theme ... */
        mSoundManager->settings = g_settings_new (UKUI_SOUND_SCHEMA);
        g_signal_connect (mSoundManager->settings, "changed",  G_CALLBACK (gsettings_notify_cb), mSoundManager);

        /* ... and we listen to changes of the theme base directories
         * in $HOME ...*/

        if ((env = getenv ("XDG_DATA_HOME")) && *env == '/')
                p = g_build_filename (env, "sounds", NULL);
        else if (((env = getenv ("HOME")) && *env == '/') || (env = g_get_home_dir ()))
                p = g_build_filename (env, ".local", "share", "sounds", NULL);
        else
                p = NULL;

        if (p) {
                mSoundManager->register_directory_callback (p, NULL);
                g_free (p);
        }

        /* ... and globally. */
        if (!(dd = getenv ("XDG_DATA_DIRS")) || *dd == 0)
                dd = "/usr/local/share:/usr/share";

        ps = g_strsplit (dd, ":", 0);

        for (k = ps; *k; ++k)
                mSoundManager->register_directory_callback (*k, NULL);

        g_strfreev (ps);
//#endif

        return true;
}

void SoundManager::SoundManagerStop ()
{
        CT_SYSLOG(LOG_DEBUG,"Stopping sound manager");

//#ifdef HAVE_PULSE
        if (mSoundManager->settings != NULL) {
                g_object_unref (mSoundManager->settings);
                mSoundManager->settings = NULL;
        }
        if (mSoundManager->timeout) {
                g_source_remove (mSoundManager->timeout);
                mSoundManager->timeout = 0;
        }
        while (mSoundManager->monitors2) {
                g_file_monitor_cancel (G_FILE_MONITOR (mSoundManager->monitors2->first()));
                g_object_unref (mSoundManager->monitors2->first());
                mSoundManager->monitors2->removeFirst();
        }
//#endif
}

SoundManager *SoundManager::SoundManagerNew ()
{
    if(nullptr == mSoundManager)
        mSoundManager = new SoundManager();
    return mSoundManager;
}
