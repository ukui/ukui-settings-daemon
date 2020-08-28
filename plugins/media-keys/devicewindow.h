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
#ifndef DEVICEWINDOW_H
#define DEVICEWINDOW_H

#include <QWidget>
#include <QString>
#include <QSvgWidget>
#include <QApplication>
#include <QX11Info>
#include <QScreen>
#include <QTimer>

namespace Ui {
class DeviceWindow;
}

class DeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceWindow(QWidget *parent = nullptr);
    ~DeviceWindow();
    void initWindowInfo();
    void setAction(const QString);
    void dialogShow();

private:
    void ensureSvgInfo(int*,int*,int*,int*);

private Q_SLOTS:
    void timeoutHandle();

private:
    Ui::DeviceWindow *ui;
    QString          mIconName;
    QSvgWidget       *mSvg;
    QTimer           *mTimer;
};

#endif // DEVICEWINDOW_H
