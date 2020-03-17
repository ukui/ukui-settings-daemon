/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Lennart Poettering <lennart@poettering.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef SOUNDPLUGIN_H
#define SOUNDPLUGIN_H

#include "soundmanager.h"
#include "plugin-interface.h"

class SoundPlugin : public PluginInterface{
public:
    ~SoundPlugin();
    static PluginInterface* getInstance();

    virtual void activate();
    virtual void deactivate();

private:
    SoundPlugin();
    SoundManager* soundManager;
    static SoundPlugin* mSoundPlugin;
};

extern "C" PluginInterface* Q_DECL_EXPORT createSettingsPlugin();
#endif /* __USD_SOUND_PLUGIN_H__ */
