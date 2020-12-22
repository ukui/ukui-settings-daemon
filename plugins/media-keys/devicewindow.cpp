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
#include "devicewindow.h"
#include "ui_devicewindow.h"
#include <QDebug>
#include <QPainter>
#include <QBitmap>

const QString allIconName[] = {
    "gpm-brightness-lcd",
    "touchpad-disabled-symbolic",
    "touchpad-enabled-symbolic",
    "media-eject",
    nullptr
};

DeviceWindow::DeviceWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceWindow)
{
    ui->setupUi(this);
}

DeviceWindow::~DeviceWindow()
{
    delete ui;
    delete mBut;
    delete mTimer;
    mBut = nullptr;
    mTimer = nullptr;
}

void DeviceWindow::initWindowInfo()
{
    int num,screenWidth,screenHeight;
    QScreen* currentScreen;

    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));

    mBut = new QPushButton(this);
    mBut->setDisabled(true);

    num = QX11Info::appScreen();                       //curent screen number 当前屏幕编号
    currentScreen = QApplication::screens().at(num);   //current screen       当前屏幕
    screenWidth = currentScreen->size().width();
    screenHeight = currentScreen->size().height();

    setFixedSize(150,140);
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint |
                   Qt::X11BypassWindowManagerHint |
                   Qt::Popup);

    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(Qt::black);
    p.setRenderHint(QPainter::Antialiasing);
    p.drawRoundedRect(bmp.rect(),6,6);
    setMask(bmp);


    setWindowOpacity(0.7);          //设置透明度
    setPalette(QPalette(Qt::black));//设置窗口背景色
    setAutoFillBackground(true);
    move((screenWidth-width())/2 , (screenHeight-height())/1.25);
}

void DeviceWindow::setAction(const QString icon)
{
    mIconName.clear();
    if("media-eject" == icon)
        mIconName = allIconName[3];
    else if("touchpad-enabled" == icon)
        mIconName = allIconName[2];
    else if("touchpad-disabled" == icon)
        mIconName = allIconName[1];
    else
        mIconName = nullptr;
}

void DeviceWindow::dialogShow()
{
    int svgWidth,svgHeight;
    int svgX,svgY;

    ensureSvgInfo(&svgWidth,&svgHeight,&svgX,&svgY);

    mBut->setFixedSize(QSize(150,140));
    mBut->setIconSize(QSize(120,110));

    mBut->setIcon(QIcon::fromTheme(mIconName));

    show();
    mTimer->start(2000);
}

void DeviceWindow::ensureSvgInfo(int* pictureWidth,int* pictureHeight,
                                 int* pictureX,int* pictureY)
{
    int width,height;               //main window size. 主窗口尺寸

    width = this->width();
    height = this->height();

    *pictureWidth = qRound(width * 0.65);
    *pictureHeight = qRound(height * 0.65);
    *pictureX = (width - *pictureWidth) / 2;
    *pictureY = (height - *pictureHeight) /2;
}

void DeviceWindow::timeoutHandle()
{
    hide();
    mTimer->stop();
}
