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
#include "xrdb-plugin.h"
#include "xrdb-manager.h"



XrdbPlugin* XrdbPlugin::mXrdbPlugin = nullptr;

XrdbPlugin::XrdbPlugin()
{
    USD_LOG(LOG_DEBUG,"XrdbPlugin initializing!");
    m_pIXdbMgr = ukuiXrdbManager::ukuiXrdbManagerNew();
}

XrdbPlugin::~XrdbPlugin() {
    USD_LOG(LOG_DEBUG,"XrdbPlugin deconstructor!");
    if (m_pIXdbMgr) {
        delete m_pIXdbMgr;
    }
    m_pIXdbMgr = nullptr;
}

void XrdbPlugin::activate () {
    GError* error;
    USD_LOG(LOG_DEBUG,"Activating xrdn plugin!");

    error = NULL;
    if(!m_pIXdbMgr->start(&error)){
        USD_LOG(LOG_DEBUG,"unable to start xrdb manager: %s",error->message);
        g_error_free(error);
    }
}

void XrdbPlugin::deactivate () {
    USD_LOG(LOG_DEBUG,"Deactivating xrdn plugin!");
    m_pIXdbMgr->stop();
}

PluginInterface* XrdbPlugin::getInstance()
{
    if(nullptr == mXrdbPlugin)
        mXrdbPlugin = new XrdbPlugin();
    return mXrdbPlugin;
}

PluginInterface* createSettingsPlugin()
{
    return XrdbPlugin::getInstance();
}

