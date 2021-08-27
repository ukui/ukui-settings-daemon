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
#include <QString>
#include <QDir>

#include "sound-manager.h"

SoundManager* SoundManager::mSoundManager = nullptr;

SoundManager::SoundManager()
{
    timer = new QTimer();
    connect(timer, &QTimer::timeout, this, &SoundManager::flush_cb);
}

SoundManager::~SoundManager()
{
    USD_LOG(LOG_DEBUG,"SoundManager destructor!");
    if(mSoundManager)
        delete mSoundManager;
}

void
sample_info_cb (pa_context *c, const pa_sample_info *i, int eol, void *userdata)
{

    pa_operation *o;
    if (!i) {

        USD_LOG(LOG_DEBUG,"can't find sample");
        return;
    }

    USD_LOG(LOG_DEBUG,"Found sample %s", i->name);

    /* We only flush those samples which have an XDG sound name
     * attached, because only those originate from themeing  */
    if (!(pa_proplist_gets (i->proplist, PA_PROP_EVENT_ID)))
        return;

    USD_LOG(LOG_DEBUG,"Dropping sample %s from cache", i->name);

    if (!(o = pa_context_remove_sample (c, i->name, NULL, NULL))) {
        USD_LOG(LOG_DEBUG,"pa_context_remove_sample (): %s", pa_strerror (pa_context_errno (c)));
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

    if (!(ml = pa_mainloop_new ())) {
        USD_LOG(LOG_DEBUG,"Failed to allocate pa_mainloop");
        goto fail;
    }

    if (!(pl = pa_proplist_new ())) {
        USD_LOG(LOG_DEBUG,"Failed to allocate pa_proplist");
        goto fail;
    }

    pa_proplist_sets (pl, PA_PROP_APPLICATION_NAME, PACKAGE_NAME);
    pa_proplist_sets (pl, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);
    pa_proplist_sets (pl, PA_PROP_APPLICATION_ID, "org.ukui.SettingsDaemon");

    if (!(c = pa_context_new_with_proplist (pa_mainloop_get_api (ml), PACKAGE_NAME, pl))) {
        USD_LOG(LOG_DEBUG,"Failed to allocate pa_context");
        goto fail;
    }

    pa_proplist_free (pl);
    pl = NULL;

    if (pa_context_connect (c, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0) {
        USD_LOG(LOG_DEBUG,"pa_context_connect(): %s", pa_strerror (pa_context_errno (c)));
        goto fail;
    }

    /* Wait until the connection is established */
    while (pa_context_get_state (c) != PA_CONTEXT_READY) {

        if (!PA_CONTEXT_IS_GOOD (pa_context_get_state (c))) {
            USD_LOG(LOG_DEBUG,"Connection failed: %s", pa_strerror (pa_context_errno (c)));
            goto fail;
        }

        if (pa_mainloop_iterate (ml, TRUE, NULL) < 0) {
            USD_LOG(LOG_DEBUG,"pa_mainloop_iterate() failed");
            goto fail;
        }
    }

    /* Enumerate all cached samples */
    if (!(o = pa_context_get_sample_info_list (c, sample_info_cb, NULL))) {
        USD_LOG(LOG_DEBUG,"pa_context_get_sample_info_list(): %s", pa_strerror (pa_context_errno (c)));
        goto fail;
    }

    /* Wait until our operation is finished and there's nothing
     * more queued to send to the server */
    while (pa_operation_get_state (o) == PA_OPERATION_RUNNING || pa_context_is_pending (c)) {

        if (!PA_CONTEXT_IS_GOOD (pa_context_get_state (c))) {
            USD_LOG(LOG_DEBUG,"Connection failed: %s", pa_strerror (pa_context_errno (c)));
            goto fail;
        }

        if (pa_mainloop_iterate (ml, TRUE, NULL) < 0) {
            USD_LOG(LOG_DEBUG,"pa_mainloop_iterate() failed");
            goto fail;
        }
    }

    USD_LOG(LOG_DEBUG,"send over...");
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
    flush_cache();
    timer->stop();

    USD_LOG(LOG_DEBUG,"sound it");
    return false;
}

void SoundManager::trigger_flush ()
{
    if(timer->isActive())
        timer->stop();

    /* We delay the flushing a bit so that we can coalesce
     * multiple changes into a single cache flush */
    timer->start(500);

    USD_LOG(LOG_DEBUG,"sound it");
}

/* func: listen for org.mate.sound
 */
void
SoundManager::gsettings_notify_cb (const QString& key)
{
    trigger_flush();
}
/*func : listen for follow directory.
 *  $HOME/.locale/share/sounds
 *  /usr/share/ukui/
 *  /usr/share/
 *  /usr/locale/share/
 */
void
SoundManager::file_monitor_changed_cb (const QString& path)
{
    trigger_flush ();
}

bool
SoundManager::register_directory_callback (const QString path,
                             GError **error)
{
    QDir dir;
    QFileSystemWatcher* w;
    bool succ = false;


    w = new QFileSystemWatcher();

    if(w->addPath(path)){
        connect(w,&QFileSystemWatcher::directoryChanged, this, &SoundManager::file_monitor_changed_cb);
        monitors->push_front(w);
        succ = true;
    }

    return succ;
}

bool SoundManager::SoundManagerStart (GError **error)
{
    const char *env, *dd;
    QString path;
    QString homePath;
    QStringList pathList;
    int pathNum;
    int i;

    USD_LOG(LOG_DEBUG,"Starting sound manager");
    monitors = new QList<QFileSystemWatcher*>();

    /* We listen for change of the selected theme ... */
    settings = new QGSettings(UKUI_SOUND_SCHEMA);

//    QObject::connect(settings, &QGSettings::changed, this, &SoundManager::gsettings_notify_cb);
    connect(settings, SIGNAL(changed(QString)), this, SLOT(gsettings_notify_cb(QString)));
    /* ... and we listen to changes of the theme base directories
     * in $HOME ...*/

    homePath = QDir::homePath();
    if ((env = getenv ("XDG_DATA_HOME")) && *env == '/') {
        path = QString(env) + "/sounds";
    }
    else if (!homePath.isEmpty()) {
        path = homePath + "/.local" + "/share" + "/sounds";
    }
    else {
        path = nullptr;
    }

    if (!path.isNull() && !path.isEmpty()) {
        USD_LOG(LOG_DEBUG,"ready register callback:%s",path.toLatin1().data());
        register_directory_callback (path, NULL);
    }

    /* ... and globally. */
    if (!(dd = getenv ("XDG_DATA_DIRS")) || *dd == 0) {
        dd = "/usr/local/share:/usr/share";
    }

    pathList = QString(dd).split(":");
    pathNum = pathList.count();

    for (i = 0; i < pathNum; ++i) {
        USD_LOG(LOG_DEBUG,"ready register callback:%s",pathList.at(i).toLatin1().data());
        register_directory_callback (pathList.at(i), NULL);
    }

    trigger_flush();
    return true;
}

void SoundManager::SoundManagerStop ()
{
    USD_LOG(LOG_DEBUG,"Stopping sound manager");

    if (settings) {
        delete settings;
        settings = nullptr;
    }

    while (monitors->length()) {
        delete monitors->first();
        monitors->removeFirst();
    }
    delete monitors;
    monitors = nullptr;
}

SoundManager *SoundManager::SoundManagerNew ()
{
    if(nullptr == mSoundManager)
        mSoundManager = new SoundManager();
    return mSoundManager;
}
