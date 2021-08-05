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
#include "clib-syslog.h"
#define DBUS_NAME       "org.ukui.SettingsDaemon"
#define DBUS_PATH       "/org/ukui/SettingsDaemon/wayland"
#define DBUS_INTERFACE  "org.ukui.SettingsDaemon.wayland"

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
    mDbusXrandInter = new QDBusInterface(DBUS_NAME,
                                         DBUS_PATH,
                                         DBUS_INTERFACE,
                                         QDBusConnection::sessionBus(), this);
     if (!mDbusXrandInter->isValid()) {
        USD_LOG(LOG_DEBUG, "stderr:%s\n",qPrintable(QDBusConnection::sessionBus().lastError().message()));
    }
    //监听dbus变化  更改主屏幕时，会进行信号发送
    connect(mDbusXrandInter, SIGNAL(screenPrimaryChanged(int,int,int,int)),
            this, SLOT(priScreenChanged(int,int,int,int)));

    mScale = getScreenGeometry("scale");
}

DeviceWindow::~DeviceWindow()
{
    delete ui;
    delete mBut;
    delete mTimer;
    mBut = nullptr;
    mTimer = nullptr;
}

/* 主屏幕变化监听函数 */
void DeviceWindow::priScreenChanged(int x, int y, int Width, int Height)
{
    move((x + Width) * mScale - width() - 200,
         (y + Height) * mScale - height() - 100);
}

int DeviceWindow::getScreenGeometry(QString methodName)
{
    int res = 0;
    QDBusMessage message = QDBusMessage::createMethodCall(DBUS_NAME,
                                                          DBUS_PATH,
                                                          DBUS_INTERFACE,
                                                          methodName);
    QDBusMessage response = QDBusConnection::sessionBus().call(message);
    if (response.type() == QDBusMessage::ReplyMessage)
    {
        if(response.arguments().isEmpty() == false) {
            int value = response.arguments().takeFirst().toInt();
            res = value;
        }
    } else {
        USD_LOG(LOG_DEBUG, "%s called failed", methodName.toLatin1().data());
    }
    return res;
}

void DeviceWindow::initWindowInfo()
{
    int x, y, screenWidth, screenHeight;

    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));

    mBut = new QPushButton(this);
    mBut->setDisabled(true);

    x = getScreenGeometry("x");
    y = getScreenGeometry("y");
    screenWidth = getScreenGeometry("width");
    screenHeight = getScreenGeometry("height");

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
    move((x + screenWidth) * mScale - width() - 200,
         (y + screenHeight) * mScale - height() - 100);
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
        mIconName = icon;
}

void DeviceWindow::dialogShow()
{
    int svgWidth,svgHeight;
    int svgX,svgY;

    ensureSvgInfo(&svgWidth,&svgHeight,&svgX,&svgY);

    mBut->setFixedSize(QSize(144,144));
    mBut->setIconSize(QSize(120,120));

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
