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
#include <QGSettings/qgsettings.h>

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
    void dialogVolumeShow();
    void dialogBrightShow();
    void setWidgetLayout();
    void setVolumeMuted(bool);
    void setVolumeLevel(int);
    void setVolumeRange(int, int);
    void setBrightIcon(const QString icon);
    void setBrightValue(int value);
    int getScreenGeometry (QString methodName);

    QPixmap drawLightColoredPixmap(const QPixmap &source, const QString &style);

private Q_SLOTS:
    void timeoutHandle();
    void priScreenChanged(int x, int y, int width, int height);
    void geometryChangedHandle();
    void onStyleChanged(const QString&);
private:
    void showEvent(QShowEvent* e);
    void paintEvent(QPaintEvent* e);

private:
    Ui::VolumeWindow *ui;
    QVBoxLayout *mVLayout;
    QHBoxLayout *mBarLayout;
    QHBoxLayout *mSvgLayout;
    QHBoxLayout *mLabLayout;
    QSpacerItem *mSpace;
    QLabel      *mLabel;
    QProgressBar *mVolumeBar;
    QProgressBar *mBrightBar;
    QLabel      *mBut;
    QTimer       *mTimer;
    QString      mIconName;
    QDBusInterface  *mDbusXrandInter;
    QGSettings      *m_styleSettings;



    double    mScale = 1.0;
    int mVolumeLevel;
    int mMaxVolume,mMinVolume;
    bool mVolumeMuted;

    int mbrightValue;

};
#endif // MEDIAKEYSWINDOW_H
