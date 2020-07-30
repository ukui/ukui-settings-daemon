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

const QString ICONDIR = "/usr/share/icons/ukui-icon-theme-default/scalable";
const QString allIconName[] = {
    ICONDIR + "/status/gpm-brightness-lcd.svg",
    ICONDIR + "/status/touchpad-disabled-symbolic.svg",
    ICONDIR + "/status/touchpad-enabled-symbolic.svg",
    ICONDIR + "/actions/media-eject.svg",
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
    delete mSvg;
    delete mTimer;
    mSvg = nullptr;
    mTimer = nullptr;
}

void DeviceWindow::initWindowInfo()
{
    int num,screenWidth,screenHeight;
    QScreen* currentScreen;

    mSvg = new QSvgWidget(this);
    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));

    num = QX11Info::appScreen();                       //curent screen number 当前屏幕编号
    currentScreen = QApplication::screens().at(num);   //current screen       当前屏幕
    screenWidth = currentScreen->size().width();
    screenHeight = currentScreen->size().height();

    setFixedSize(190,190);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowOpacity(0.7);          //设置透明度
    setPalette(QPalette(Qt::black));//设置窗口背景色
    setAutoFillBackground(true);
    move((screenWidth-width())/2 , (screenHeight-height())/2);
}

void DeviceWindow::setAction(const QString icon)
{
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

    mSvg->setGeometry(svgX,svgY,svgWidth,svgHeight);
    mSvg->load(mIconName);
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
