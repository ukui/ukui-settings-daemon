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
#include "a11ysettingsmanager.h"
#include "clib-syslog.h"

A11ySettingsManager* A11ySettingsManager::mA11ySettingsManager = nullptr;

A11ySettingsManager::A11ySettingsManager()
{
    interface_settings = new QGSettings("org.mate.interface");
    a11y_apps_settings = new QGSettings("org.gnome.desktop.a11y.applications");
}

A11ySettingsManager::~A11ySettingsManager()
{
    delete interface_settings;
    delete a11y_apps_settings;
}

A11ySettingsManager* A11ySettingsManager::A11ySettingsManagerNew()
{
    if(nullptr == mA11ySettingsManager)
        mA11ySettingsManager = new A11ySettingsManager();
    return mA11ySettingsManager;
}


void A11ySettingsManager::AppsSettingsChanged(QString key){
    bool screen_reader,keyboard;

    if((key == "screen-reader-enabled") == false &&
       (key == "screen-keyboard-enabled") == false)
        return;

    qDebug("screen reader or OSK enabledment changed");

    screen_reader = a11y_apps_settings->get("screen-reader-enabled").toBool();
    keyboard = a11y_apps_settings->get("screen-keyboard-enabled").toBool();

    if(screen_reader || keyboard){
        qDebug("Enabling accessibility,screen reader or OSK enabled!");
        interface_settings->set("accessibility",true);
    }else if((screen_reader == false) && (keyboard == false)){
        qDebug("Disabling accessibility,screen reader or OSK disabled!");
        interface_settings->set("accessibility",false);
    }
}

bool A11ySettingsManager::A11ySettingsManagerStart()
{
    qDebug("Starting a11y_settings manager!");

    connect(a11y_apps_settings, SIGNAL(changed(QString)),
                          this, SLOT(AppsSettingsChanged(QString)));

    /* If any of the screen reader or on-screen keyboard are enabled,
     * make sure a11y is enabled for the toolkits.
     * We don't do the same thing for the reverse so it's possible to
     * enable AT-SPI for the toolkits without using an a11y app */
    if(a11y_apps_settings->get("screen-keyboard-enabled").toBool()||
       a11y_apps_settings->get("screen-reader-enabled").toBool())
        interface_settings->set("accessibility",true);
    return true;
}

void A11ySettingsManager::A11ySettingsMAnagerStop()
{
    CT_SYSLOG(LOG_DEBUG,"Stopping a11y_settings manager");
}
