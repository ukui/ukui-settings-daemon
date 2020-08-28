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
#include "backgroundplugin.h"
#include "clib-syslog.h"

BackgroundManager* BackgroundPlugin::mManager = nullptr;
PluginInterface* BackgroundPlugin::mInstance = nullptr;

BackgroundPlugin::BackgroundPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "background plugin init...");
    if (nullptr == mManager) {
        mManager = BackgroundManager::getInstance();
    }
}

BackgroundPlugin::~BackgroundPlugin()
{
    CT_SYSLOG(LOG_DEBUG, "background plugin free...");
    if (nullptr != mManager) {delete mManager; mManager = nullptr;}
}

PluginInterface *BackgroundPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new BackgroundPlugin();
    }

    return mInstance;
}

void BackgroundPlugin::activate()
{
    CT_SYSLOG (LOG_DEBUG, "Activating background plugin");
    if (!mManager->managerStart()) {
        CT_SYSLOG(LOG_ERR, "Unable to start background manager!");
    }
}

void BackgroundPlugin::deactivate()
{
    CT_SYSLOG (LOG_DEBUG, "Deactivating background plugin");
    if (nullptr != mManager) {
        mManager->managerStop();
    }
}

PluginInterface* createSettingsPlugin()
{
    return BackgroundPlugin::getInstance();
}
