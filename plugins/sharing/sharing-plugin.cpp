/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2021 KylinSoft Co., Ltd.
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
#include "sharing-plugin.h"

SharingPlugin* SharingPlugin::mSharingPlugin = nullptr;

SharingPlugin::SharingPlugin()
{
    USD_LOG(LOG_DEBUG,"SharingPlugin initializing!");

    mSharingManager = SharingManager::SharingManagerNew();
}

SharingPlugin::~SharingPlugin() {
    USD_LOG(LOG_DEBUG,"SharingPlugin deconstructor!");
    if (mSharingManager) {
        delete mSharingManager;
    }
    mSharingManager = nullptr;
}

void SharingPlugin::activate () {
    USD_LOG (LOG_DEBUG, "Activating %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);
    if(!mSharingManager->start()){
        USD_LOG(LOG_DEBUG,"unable to start sharing manager");
    }
}

void SharingPlugin::deactivate () {
    USD_LOG(LOG_DEBUG,"Deactivating sharing plugin!");
    mSharingManager->stop();
}

PluginInterface* SharingPlugin::getInstance()
{
    if(nullptr == mSharingPlugin)
        mSharingPlugin = new SharingPlugin();
    return mSharingPlugin;
}

PluginInterface* createSettingsPlugin()
{
    return SharingPlugin::getInstance();
}

