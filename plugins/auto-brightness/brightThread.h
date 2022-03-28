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

#ifndef BRIGHTTHREAD_H
#define BRIGHTTHREAD_H

#include <QThread>
#include <QMutex>
#include "QGSettings/qgsettings.h"

class BrightThread : public QThread
{
    Q_OBJECT
public:
    BrightThread(QObject *parent = nullptr);
    ~BrightThread();
    void stopImmediately();
    void setBrightness(int brightness);
    int  getRealTimeBrightness();
protected:
    void run();

private:


    int m_delayms;
    bool m_exit;
    int m_destBrightness;

    QMutex m_lock;
    QGSettings *m_powerSettings;
    QGSettings *m_brightnessSettings;

};

#endif // BRIGHTTHREAD_H
