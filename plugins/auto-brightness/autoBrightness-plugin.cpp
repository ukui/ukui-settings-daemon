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
#include "autoBrightness-plugin.h"

PluginInterface *AutoBrightnessPlugin::mInstance       = nullptr;
AutoBrightnessManager *AutoBrightnessPlugin::mAutoBrightnessManager = nullptr;

AutoBrightnessPlugin::AutoBrightnessPlugin()
{
    qDebug("AutoBrightness Plugin initializing");
    if(nullptr == mAutoBrightnessManager)
        mAutoBrightnessManager = AutoBrightnessManager::AutoBrightnessManagerNew();
}

AutoBrightnessPlugin::~AutoBrightnessPlugin()
{
    if(mAutoBrightnessManager)
        delete mAutoBrightnessManager;
    if(mInstance)
        delete mInstance;
}

void AutoBrightnessPlugin::activate()
{
    USD_LOG(LOG_DEBUG,"Activating AutoBrightness plugins");
    bool res;
    res = mAutoBrightnessManager->AutoBrightnessManagerStart();
    if(!res)
        USD_LOG(LOG_ERR,"Unable to start AutoBrightness manager");
}

PluginInterface *AutoBrightnessPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new AutoBrightnessPlugin();

    return mInstance;
}

void AutoBrightnessPlugin::deactivate()
{
    qDebug("Deactivating AutoBrightness plugin");
    mAutoBrightnessManager->AutoBrightnessManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return AutoBrightnessPlugin::getInstance();
}

