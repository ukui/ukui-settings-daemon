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
#include "keyboard-plugin.h"
#include "clib-syslog.h"

PluginInterface * KeyboardPlugin::mInstance=nullptr;
KeyboardManager * KeyboardPlugin::UsdKeyboardManager=nullptr;

KeyboardPlugin::KeyboardPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"KeyboardPlugin initializing!");
    if(nullptr == UsdKeyboardManager)
        UsdKeyboardManager = KeyboardManager::KeyboardManagerNew();
}

KeyboardPlugin::~KeyboardPlugin()
{
    CT_SYSLOG(LOG_DEBUG,"Keyboard plugin free");
    if (UsdKeyboardManager){
        delete UsdKeyboardManager;
        UsdKeyboardManager =nullptr;
    }
}

void KeyboardPlugin::activate()
{
    bool res;
    CT_SYSLOG(LOG_DEBUG,"Activating Keyboard Plugin");
    res = UsdKeyboardManager->KeyboardManagerStart();
    if(!res){
        CT_SYSLOG(LOG_ERR,"Unable to start Keyboard Manager!")
    }

}

PluginInterface * KeyboardPlugin::getInstance()
{
    if(nullptr == mInstance){
        mInstance = new KeyboardPlugin();
    }
    return  mInstance;
}

void KeyboardPlugin::deactivate()
{
    CT_SYSLOG(LOG_DEBUG,"Deactivating Keyboard Plugin");
    UsdKeyboardManager->KeyboardManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return KeyboardPlugin::getInstance();
}


