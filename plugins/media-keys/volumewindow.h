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
#ifndef MEDIAKEYSWINDOW_H
#define MEDIAKEYSWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QProgressBar>
#include <QSvgWidget>
#include <QTimer>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {class VolumeWindow;}
QT_END_NAMESPACE

class VolumeWindow : public QWidget
{
    Q_OBJECT

public:
    VolumeWindow(QWidget *parent = nullptr);
    ~VolumeWindow();
    void initWindowInfo();
    void dialogShow();
    void setWidgetLayout();
    void setVolumeMuted(bool);
    void setVolumeLevel(int);
    void setVolumeRange(int, int);

private Q_SLOTS:
    void timeoutHandle();

private:
    Ui::VolumeWindow *ui;
    QVBoxLayout *mVLayout;
    QHBoxLayout *mBarLayout;
    QHBoxLayout *mSvgLayout;
    QSpacerItem *mSpace;
    QProgressBar *mBar;
    QSvgWidget   *mSvg;
    QTimer       *mTimer;
    QString      mIconName;

    int mVolumeLevel;
    int mMaxVolume,mMinVolume;
    bool mVolumeMuted;
};
#endif // MEDIAKEYSWINDOW_H
