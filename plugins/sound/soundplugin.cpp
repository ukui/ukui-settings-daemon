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
#include "soundplugin.h"
#include "clib-syslog.h"

SoundPlugin* SoundPlugin::mSoundPlugin = nullptr;
SoundPlugin::SoundPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"UsdSoundPlugin initializing!");
    soundManager = SoundManager::SoundManagerNew();
}

SoundPlugin::~SoundPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"UsdSoundPlugin deconstructor!");
    if(soundManager)
        delete soundManager;
}

void SoundPlugin::activate ()
{
        GError *error = NULL;
        CT_SYSLOG(LOG_DEBUG,"Activating sound plugin!");

        if (!soundManager->SoundManagerStart(&error)) {
                CT_SYSLOG(LOG_DEBUG,"Unable to start sound manager: %s", error->message);
                g_error_free (error);
        }
}

void SoundPlugin::deactivate ()
{
        CT_SYSLOG(LOG_DEBUG,"Deactivating sound plugin!");
        soundManager->SoundManagerStop();
}

PluginInterface* SoundPlugin::getInstance()
{
    if(nullptr == mSoundPlugin)
        mSoundPlugin = new SoundPlugin();
    return mSoundPlugin;
}

PluginInterface* createSettingsPlugin()
{
    return SoundPlugin::getInstance();
}

