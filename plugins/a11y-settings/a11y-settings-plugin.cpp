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
#include "a11y-settings-plugin.h"
#include "clib-syslog.h"

PluginInterface* A11ySettingsPlugin::mInstance = nullptr;

A11ySettingsPlugin::A11ySettingsPlugin()
{
    USD_LOG(LOG_DEBUG,"A11SettingsPlugin initializing!");
    settingsManager=A11ySettingsManager::A11ySettingsManagerNew();
}

A11ySettingsPlugin::~A11ySettingsPlugin()
{
    if (settingsManager)
        delete settingsManager;
}

void A11ySettingsPlugin::activate()
{
    bool res;

    USD_LOG(LOG_DEBUG,"Activating a11y-settings plugincompilation time:[%s] [%s]",__DATE__,__TIME__);
    res=settingsManager->A11ySettingsManagerStart();
    if(!res){
        USD_LOG(LOG_WARNING,"Unable to start a11y-settings manager!");
    }
}

PluginInterface *A11ySettingsPlugin::getInstance()
{
    if (nullptr == mInstance) {
        mInstance = new A11ySettingsPlugin();
    }
    return mInstance;
}

void A11ySettingsPlugin::deactivate()
{
    USD_LOG(LOG_DEBUG,"Deactivating a11y-settings plugin!");
    settingsManager->A11ySettingsMAnagerStop();
}

PluginInterface* createSettingsPlugin()
{
    return A11ySettingsPlugin::getInstance();
}
