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


#include "clib-syslog.h"
#include "autoBrightness-plugin.h"

PluginInterface *AutoBrightnessPlugin::m_instance       = nullptr;
AutoBrightnessManager *AutoBrightnessPlugin::m_autoBrightnessManager = nullptr;

AutoBrightnessPlugin::AutoBrightnessPlugin()
{
    USD_LOG(LOG_DEBUG,"AutoBrightness Plugin initializing");
    if(nullptr == m_autoBrightnessManager)
        m_autoBrightnessManager = AutoBrightnessManager::autoBrightnessManagerNew();
}

AutoBrightnessPlugin::~AutoBrightnessPlugin()
{
    if(m_autoBrightnessManager) {
        delete m_autoBrightnessManager;
    }

    if(m_instance) {
        delete m_instance;
    }
}

void AutoBrightnessPlugin::activate()
{
    bool res;
    USD_LOG(LOG_DEBUG,"Activating AutoBrightness plugins");
    res = m_autoBrightnessManager->autoBrightnessManagerStart();

    if(!res) {
        USD_LOG(LOG_ERR,"Unable to start AutoBrightness manager");
    }
}

PluginInterface *AutoBrightnessPlugin::getInstance()
{
    if(nullptr == m_instance) {
        m_instance = new AutoBrightnessPlugin();
    }

    return m_instance;
}

void AutoBrightnessPlugin::deactivate()
{
    USD_LOG(LOG_DEBUG, "Deactivating AutoBrightness plugin");
    m_autoBrightnessManager->autoBrightnessManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return AutoBrightnessPlugin::getInstance();
}

