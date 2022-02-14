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

#include "a11y-keyboard-plugin.h"
#include "clib-syslog.h"

PluginInterface *A11yKeyboardPlugin::mInstance = nullptr;
A11yKeyboardManager *A11yKeyboardPlugin::UsdA11yManager= nullptr;

A11yKeyboardPlugin::A11yKeyboardPlugin()
{
    USD_LOG(LOG_DEBUG,"A11yKeyboardPlugin initializing ");
    if(nullptr == UsdA11yManager)
        UsdA11yManager = A11yKeyboardManager::A11KeyboardManagerNew();

}

A11yKeyboardPlugin::~A11yKeyboardPlugin()
{
    if(UsdA11yManager){
        delete UsdA11yManager;
        UsdA11yManager = nullptr;
    }
}

void A11yKeyboardPlugin::activate()
{
    bool res;
    USD_LOG (LOG_DEBUG, "Activating %s plugin compilation time:[%s] [%s]",MODULE_NAME,__DATE__,__TIME__);
    res = UsdA11yManager->A11yKeyboardManagerStart();
    if(!res)
        USD_LOG(LOG_ERR,"Unable to start A11y-Keyboard manager");
}

PluginInterface * A11yKeyboardPlugin::getInstance()
{
    if(nullptr == mInstance)
        mInstance = new A11yKeyboardPlugin();

    return mInstance;
}

void A11yKeyboardPlugin::deactivate()
{
    USD_LOG(LOG_DEBUG,"Deactivating A11y-Keyboard plugin");
    UsdA11yManager->A11yKeyboardManagerStop();
}

PluginInterface *createSettingsPlugin()
{
    return A11yKeyboardPlugin::getInstance();
}
