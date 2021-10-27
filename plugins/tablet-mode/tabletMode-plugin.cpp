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
#include "tabletMode-plugin.h"
#include "clib-syslog.h"

PluginInterface *TabletModePlugin::mInstance       = nullptr;
TabletModeManager *TabletModePlugin::mTableManager = nullptr;

TabletModePlugin::TabletModePlugin()
{
    qDebug("TabletMode Plugin initializing");
    if(nullptr == mTableManager)
        mTableManager = TabletModeManager::TabletModeManagerNew();
}

TabletModePlugin::~TabletModePlugin()
{
    if(mTableManager) {
        delete mTableManager;
        mTableManager = nullptr;
    }
}

void TabletModePlugin::activate()
{
    USD_LOG (LOG_DEBUG, "Activating %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);

    bool res;
    res = mTableManager->TabletModeManagerStart();
    if(!res)
        qWarning("Unable to start Tablet manager");
}

PluginInterface *TabletModePlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new TabletModePlugin();

    return mInstance;
}

void TabletModePlugin::deactivate()
{
    qDebug("Deactivating Xrandr plugin");
    mTableManager->TabletModeManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return TabletModePlugin::getInstance();
}

