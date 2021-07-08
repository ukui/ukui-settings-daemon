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
#include "mediakey-plugin.h"
#include "clib-syslog.h"

PluginInterface* MediakeyPlugin::mInstance = nullptr;

MediakeyPlugin::MediakeyPlugin()
{
    USD_LOG(LOG_ERR, "mediakey plugin init...");
    mManager = MediaKeysManager::mediaKeysNew();
}

MediakeyPlugin::~MediakeyPlugin()
{
    USD_LOG(LOG_ERR,"MediakeyPlugin deconstructor!");
    if(mManager){
        delete mManager;
        mManager = nullptr;
    }
}

PluginInterface *MediakeyPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new MediakeyPlugin();
    }

    return mInstance;
}

void MediakeyPlugin::activate()
{
    GError *error = NULL;
    USD_LOG(LOG_ERR, "activating mediakey plugin ...");

    if (!mManager->mediaKeysStart(error)) {
            USD_LOG(LOG_DEBUG,"Unable to start media-keys manager: %s", error->message);
            g_error_free (error);
    }
}

void MediakeyPlugin::deactivate()
{
    USD_LOG(LOG_ERR, "deactivating mediakey plugin ...");
    mManager->mediaKeysStop();
}

PluginInterface* createSettingsPlugin()
{
    return MediakeyPlugin::getInstance();
}
