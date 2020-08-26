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
#include "keybindings-plugin.h"
#include "clib-syslog.h"

PluginInterface    *KeybindingsPlugin::mInstance=nullptr;
KeybindingsManager *KeybindingsPlugin::mKeyManager=nullptr;

KeybindingsPlugin::KeybindingsPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"KeybindingsPlugin initializing");
    if(nullptr == mKeyManager)
        mKeyManager = KeybindingsManager::KeybindingsManagerNew();
}

KeybindingsPlugin::~KeybindingsPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"KeybindingsPlugin free");
    if(mKeyManager){
        delete mKeyManager;
        mKeyManager = nullptr;
    }
}

void KeybindingsPlugin::activate()
{
    bool res;
    CT_SYSLOG(LOG_DEBUG,"Activating Keybindings");
    res = mKeyManager->KeybindingsManagerStart();
    if(!res)
        CT_SYSLOG(LOG_ERR,"Unable to start Keybindings manager");
}

PluginInterface *KeybindingsPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new KeybindingsPlugin();
    return mInstance;
}

void KeybindingsPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Dectivating Keybindings Plugin");
    mKeyManager->KeybindingsManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return KeybindingsPlugin::getInstance();
}
