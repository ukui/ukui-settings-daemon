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
#include <QPushButton>
#include <QLabel>
#include <QObject>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>

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
    int getScreenGeometry (QString methodName);

    QPixmap drawLightColoredPixmap(const QPixmap &source);

private Q_SLOTS:
    void timeoutHandle();
    void priScreenChanged(int x, int y, int width, int height);

private:
    Ui::VolumeWindow *ui;
    QVBoxLayout *mVLayout;
    QHBoxLayout *mBarLayout;
    QHBoxLayout *mSvgLayout;
    QHBoxLayout *mLabLayout;
    QSpacerItem *mSpace;
    QLabel      *mLabel;
    QProgressBar *mBar;
    QPushButton  *mBut;
    QTimer       *mTimer;
    QString      mIconName;
    QDBusInterface  *mDbusXrandInter;

    double    mScale;
    int mVolumeLevel;
    int mMaxVolume,mMinVolume;
    bool mVolumeMuted;
};
#endif // MEDIAKEYSWINDOW_H
