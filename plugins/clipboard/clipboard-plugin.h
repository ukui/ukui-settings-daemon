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
#ifndef CLIPBOARDPLUGIN_H
#define CLIPBOARDPLUGIN_H
#include "clipboard-manager.h"
#include "plugin-interface.h"

#include <QtCore/QtGlobal>

class ClipboardPlugin : public PluginInterface
{
public:
    ~ClipboardPlugin();
    static PluginInterface* getInstance();

    virtual void activate ();
    virtual void deactivate ();

private:
    ClipboardPlugin();
    ClipboardPlugin(ClipboardPlugin&)=delete;

private:
    static ClipboardManager*        mManager;
    static PluginInterface*         mInstance;
};

extern "C" Q_DECL_EXPORT PluginInterface* createSettingsPlugin();

#endif // CLIPBOARDPLUGIN_H
