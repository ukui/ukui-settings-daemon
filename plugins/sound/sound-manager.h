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
#ifndef SOUNDMANAGER_H
#define SOUNDMANAGER_H

#include <QObject>
#include <QList>
#include <QTimer>
#include <QFileSystemWatcher>
#include "QGSettings/qgsettings.h"

#ifdef signals
#undef signals
#endif

extern "C"{
#include <gio/gio.h>
}

#define UKUI_SOUND_SCHEMA "org.mate.sound"
#define PACKAGE_NAME "ukui-settings-daemon"
#define PACKAGE_VERSION "1.1.1"

class SoundManager : public QObject{
    Q_OBJECT
public:
    ~SoundManager();
    static SoundManager* SoundManagerNew();
    bool SoundManagerStart(GError **error);
    void SoundManagerStop();
private:
    SoundManager();

    void trigger_flush();
    bool register_directory_callback(const QString path,
                                         GError **error);

private Q_SLOTS:
    bool flush_cb();
    void gsettings_notify_cb (const QString& key);
    void file_monitor_changed_cb(const QString& path);

private:
    static SoundManager* mSoundManager;
    QGSettings* settings;
    QList<QFileSystemWatcher*>* monitors;
    QTimer* timer;
};

#endif /* SOUNDMANAGER_H */
