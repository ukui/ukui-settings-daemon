#include "typingbreakmanager.h"
#include "clib-syslog.h"
#include <glib.h>

TypingBreakManager* TypingBreakManager::mTypingBreakManager =nullptr;

TypingBreakManager::TypingBreakManager()
{

}

TypingBreakManager::~TypingBreakManager()
{

}

TypingBreakManager* TypingBreakManager::TypingBreakManagerNew()
{
    if(nullptr == mTypingBreakManager)
        mTypingBreakManager= new TypingBreakManager();
    return mTypingBreakManager;
}

bool TypingBreakManager::TypingBreakManagerStart(GError **error)
{
    gboolean enabled;
    CT_SYSLOG(LOG_DEBUG,"Starting typing_break manager!");

    settings=g_settings_new(UKUI_BREAK_SCHMA);
    g_signal_connect(settings,"changed::enabled",
                     G_CALLBACK(typing_break_enabled_callback),mTypingBreakManager);

    enabled=g_settings_get_boolean(settings,"enabled");
    if(enabled)
        setup_id=g_timeout_add_seconds(3,(GSourceFunc)really_setup_typing_break,
                                       mTypingBreakManager);

    return true;
}

void TypingBreakManager::TypingBreakManagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"Stopping typing_break manager");

    if(setup_id != 0){
        g_source_remove(setup_id);
        setup_id=0;
    }
    if(child_watch_id !=0){
        g_source_remove(child_watch_id);
        child_watch_id=0;
    }
    if(typing_monitor_idle_id !=0){
        g_source_remove(typing_monitor_idle_id);
        typing_monitor_idle_id=0;
    }
    if(typing_monitor_pid > 0){
        kill(typing_monitor_pid,SIGKILL);
        g_spawn_close_pid(typing_monitor_pid);
        typing_monitor_pid=0;
    }
    if(settings){
        g_object_unref(settings);
    }
}

void TypingBreakManager::typing_break_enabled_callback(GSettings *settings,
                                   gchar* key,
                                   TypingBreakManager* manager)
{
    setup_typing_break(g_settings_get_boolean(settings,key));
}

bool TypingBreakManager::really_setup_typing_break()
{
    setup_typing_break(true);
    mTypingBreakManager->setup_id=0;
    return false;
}

gboolean TypingBreakManager::typing_break_timeout()
{
    if(mTypingBreakManager->typing_monitor_pid > 0)
        kill(mTypingBreakManager->typing_monitor_pid,SIGKILL);
    mTypingBreakManager->typing_monitor_idle_id=0;
}

void TypingBreakManager::child_watch(int pid,int status)
{
    if(pid == mTypingBreakManager->typing_monitor_pid)
        mTypingBreakManager->typing_monitor_pid = 0;
    g_spawn_close_pid(pid);
}

void TypingBreakManager::setup_typing_break(gboolean enabled)
{
    if(!enabled){
        if(mTypingBreakManager->typing_monitor_pid != 0)
            mTypingBreakManager->typing_monitor_idle_id=
                    g_timeout_add_seconds(3,(GSourceFunc)typing_break_timeout,
                                          mTypingBreakManager);
        return;
    }
    if(mTypingBreakManager->typing_monitor_idle_id != 0){
        g_source_remove(mTypingBreakManager->typing_monitor_idle_id);
        mTypingBreakManager->typing_monitor_idle_id=0;
    }
    if (mTypingBreakManager->typing_monitor_pid == 0) {
        GError  *error;
        char    *argv[] = { "mate-typing-monitor", "-n", NULL };

        GSpawnFlags flags=GSpawnFlags(G_SPAWN_STDOUT_TO_DEV_NULL
                | G_SPAWN_STDERR_TO_DEV_NULL
                | G_SPAWN_SEARCH_PATH
                | G_SPAWN_DO_NOT_REAP_CHILD);

        gboolean res;
        error = NULL;
        res = g_spawn_async ("/",
                         argv,
                         NULL,
                         flags,
                         NULL,
                         NULL,
                         &mTypingBreakManager->typing_monitor_pid,
                         &error);
        if (! res) {
            /* FIXME: put up a warning */
            g_warning ("failed: %s\n", error->message);
            g_error_free (error);
            mTypingBreakManager->typing_monitor_pid = 0;
            return;
        }

        mTypingBreakManager->child_watch_id = g_child_watch_add (mTypingBreakManager->typing_monitor_pid,
                                               (GChildWatchFunc)child_watch,
                                              mTypingBreakManager);
        }
}
