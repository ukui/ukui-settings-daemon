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
#ifndef KEYBOARDMANAGER_H
#define KEYBOARDMANAGER_H

#include <QObject>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWidget>

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>

#include <QTimer>
#include <QtX11Extras/QX11Info>

#include <QGSettings/qgsettings.h>
#include <QApplication>

#include "xeventmonitor.h"
#include "keyboard-xkb.h"
#include "keyboard-widget.h"

#ifdef HAVE_X11_EXTENSIONS_XF86MISC_H
#include <X11/extensions/xf86misc.h>
#endif

#ifdef HAVE_X11_EXTENSIONS_XKB_H
#include <X11/XKBlib.h>
#include <X11/keysym.h>
#endif

class KeyboardXkb;
class KeyboardManager : public QObject
{
    Q_OBJECT
private:
    KeyboardManager()=delete;
    KeyboardManager(KeyboardManager&)=delete;
    KeyboardManager&operator=(const KeyboardManager&)=delete;
    KeyboardManager(QObject *parent = nullptr);


public:
    ~KeyboardManager();
    static KeyboardManager *KeyboardManagerNew();
    bool KeyboardManagerStart();
    void KeyboardManagerStop ();
    void usd_keyboard_manager_apply_settings(KeyboardManager *manager);
    void numlock_install_xkb_callback ();

public Q_SLOTS:
    void start_keyboard_idle_cb ();
    void apply_settings  (QString);
    void XkbEventsFilter(int keyCode);

private:
    friend void numlock_xkb_init (KeyboardManager *manager);
    friend void apply_bell       (KeyboardManager *manager);
    friend void apply_numlock    (KeyboardManager *manager);
    friend void apply_repeat     (KeyboardManager *manager);

    friend void numlock_install_xkb_callback (KeyboardManager *manager);

private:
    QTimer                 *time;
    static KeyboardManager *mKeyboardManager;
    static KeyboardXkb     *mKeyXkb;
    bool                    have_xkb;
    int                     xkb_event_base;
    QGSettings             *settings;
    QGSettings             *ksettings;
    int                     old_state;
    bool                    stInstalled;

    KeyboardWidget*         m_capsWidget;

    QDBusInterface * ifaceScreenSaver;
};

#endif // KEYBOARDMANAGER_H
