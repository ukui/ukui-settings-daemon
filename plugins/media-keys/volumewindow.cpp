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
#include "volumewindow.h"
#include "ui_volumewindow.h"
#include <QPalette>
#include <QSize>
#include <QRect>
#include <QScreen>
#include <QX11Info>
#include <QDebug>
#include <QPainter>
#include <QBitmap>
#include <KWindowEffects>
#include <QPainterPath>
#include <QPainter>
#include <QGSettings/qgsettings.h>
#include "clib-syslog.h"
#include "usd_global_define.h"
#define DBUS_NAME       DBUS_XRANDR_NAME
#define DBUS_PATH       DBUS_XRANDR_PATH
#define DBUS_INTERFACE  DBUS_XRANDR_INTERFACE

#define QT_THEME_SCHEMA   "org.ukui.style"
#define ICON_SIZE 24

const QString allIconName[] = {
    "audio-volume-muted-symbolic",
    "audio-volume-low-symbolic",
    "audio-volume-medium-symbolic",
    "audio-volume-high-symbolic",
    nullptr
};

VolumeWindow::VolumeWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VolumeWindow)
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
    m_styleSettings = new QGSettings(QT_THEME_SCHEMA);
    connect(m_styleSettings,SIGNAL(changed(const QString&)),
            this,SLOT(onStyleChanged(const QString&)));

    QGSettings *settings = new QGSettings("org.ukui.SettingsDaemon.plugins.xsettings");

    if (settings) {
        mScale = settings->get("scaling-factor").toDouble();
        mScale = mScale<1? 1.0:mScale;
    }


    delete settings;
}

VolumeWindow::~VolumeWindow()
{
    delete ui;
    if (mBarLayout)
        delete mBarLayout;
    if (mSvgLayout)
        delete mSvgLayout;
    if (mBut)
        delete mBut;
    if (mVolumeBar)
        delete mVolumeBar;
    if (mVLayout)
        delete mVLayout;
    if (mTimer)
        delete mTimer;
}

int VolumeWindow::getScreenGeometry(QString methodName)
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

/* 主屏幕变化监听函数 */
void VolumeWindow::priScreenChanged(int x, int y, int width, int height)
{
    int ax,ay;
    ax = x + (width*0.01*mScale);
    ay = y + (height*0.04*mScale);
    move(ax, ay);
}


void VolumeWindow::geometryChangedHandle()
{
    int x=QApplication::primaryScreen()->geometry().x();
    int y=QApplication::primaryScreen()->geometry().y();
    int width = QApplication::primaryScreen()->size().width();
    int height = QApplication::primaryScreen()->size().height();

    priScreenChanged(x,y,width,height);
}

void VolumeWindow::onStyleChanged(const QString& style)
{
    if(style == "icon-theme-name") {
        QSize iconSize(ICON_SIZE * mScale,ICON_SIZE * mScale);
        QIcon::setThemeName(m_styleSettings->get("icon-theme-name").toString());
        mBut->setPixmap(drawLightColoredPixmap((QIcon::fromTheme(mIconName).pixmap(iconSize))
                                               ,m_styleSettings->get("style-name").toString()));
    } else if(style == "style-name") {
        if(!this->isHidden()) {
            hide();
            show();
        }
    }
}



void VolumeWindow::initWindowInfo()
{

    connect(QApplication::primaryScreen(), &QScreen::geometryChanged, this, &VolumeWindow::geometryChangedHandle);
    connect(static_cast<QApplication *>(QCoreApplication::instance()),
              &QApplication::primaryScreenChanged, this, &VolumeWindow::geometryChangedHandle);

    //窗口性质
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

    setAttribute(Qt::WA_TranslucentBackground, true);

    setFixedSize(QSize(64,300) * mScale);

    mVolumeBar = new QProgressBar(this);
    mBrightBar = new QProgressBar(this);
    mBut = new QLabel(this);

    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));
    mVolumeLevel = 0;
    mVolumeMuted = false;
    geometryChangedHandle();//had move action

    setWidgetLayout();

}

//上下留出10个空间,音量条与svg图片之间留出10个空间
void VolumeWindow::setWidgetLayout()
{
    //窗口性质
    setFixedSize(QSize(64,300) * mScale);
    //button图片操作
    mBut->setFixedSize(QSize(31,24) * mScale);
    mBut->setAlignment(Qt::AlignCenter);
    mBut->move(17 * mScale , 253 * mScale);
    //音量条操作
    mVolumeBar->setOrientation(Qt::Vertical);
    mVolumeBar->setFixedSize(QSize(6,200) * mScale);
    mVolumeBar->move(29 * mScale,37 * mScale);
    mVolumeBar->setTextVisible(false);
    mVolumeBar->hide();
    //亮度条操作
    mBrightBar->setOrientation(Qt::Vertical);
    mBrightBar->setFixedSize(QSize(6,200) * mScale);
    mBrightBar->move(29 * mScale,37 * mScale);
    mBrightBar->setTextVisible(false);
    mBrightBar->hide();
}

int doubleToInt(double d)
{
    int I = d;
    if(d - I >= 0.5)
        return I+1;
    else
        return I;
}

QPixmap VolumeWindow::drawLightColoredPixmap(const QPixmap &source, const QString &style)
{

    int value = 255;
    if(style == "ukui-light")
    {
        value = 0;
    }

    QColor gray(255,255,255);
    QColor standard (0,0,0);
    QImage img = source.toImage();
    for (int x = 0; x < img.width(); x++) {
        for (int y = 0; y < img.height(); y++) {
            auto color = img.pixelColor(x, y);
            if (color.alpha() > 0) {
                if (qAbs(color.red()-gray.red())<20 && qAbs(color.green()-gray.green())<20 && qAbs(color.blue()-gray.blue())<20) {
                    color.setRed(value);
                    color.setGreen(value);
                    color.setBlue(value);
                    img.setPixelColor(x, y, color);
                }
                else {
                    color.setRed(value);
                    color.setGreen(value);
                    color.setBlue(value);
                    img.setPixelColor(x, y, color);
                }
            }
        }
    }
    return QPixmap::fromImage(img);
}

void VolumeWindow::dialogVolumeShow()
{
    geometryChangedHandle();
    mBrightBar->hide();
    mVolumeBar->show();
    QSize iconSize(ICON_SIZE * mScale,ICON_SIZE * mScale);
    mBut->setPixmap(drawLightColoredPixmap((QIcon::fromTheme(mIconName).pixmap(iconSize)),m_styleSettings->get("style-name").toString()));
    show();
    mTimer->start(2000);
}

void VolumeWindow::dialogBrightShow()
{
    geometryChangedHandle();
    mVolumeBar->hide();
    mBrightBar->show();
    mBrightBar->setValue(mbrightValue);

    QSize iconSize(ICON_SIZE * mScale,ICON_SIZE * mScale);

    mBut->setPixmap(drawLightColoredPixmap((QIcon::fromTheme(mIconName).pixmap(iconSize)),m_styleSettings->get("style-name").toString()));
    show();
    mTimer->start(2000);
}

void VolumeWindow::setVolumeMuted(bool muted)
{
    if(this->mVolumeMuted != muted) {
        mVolumeMuted = muted;
    }
}

void VolumeWindow::setVolumeLevel(int level)
{
    double percentage;

    mIconName.clear();
    mVolumeLevel = level;

    mVolumeBar->setValue(mVolumeLevel/(mMaxVolume/100));

    if(mVolumeMuted){
        mIconName = allIconName[0];
        return;
    }

    percentage = double(mVolumeLevel-mMinVolume) / (mMaxVolume-mMinVolume);

    if(0 <= percentage && percentage <= 0.01)
        mIconName = allIconName[0];
    if(percentage <= 0.33)
        mIconName = allIconName[1];
    else if(percentage <= 0.66)
        mIconName = allIconName[2];
    else
        mIconName = allIconName[3];
}

void VolumeWindow::setVolumeRange(int min, int max)
{
    if(min == mMinVolume && max == mMaxVolume)
        return ;

    mMaxVolume = max;
    mMinVolume = min;
    mVolumeBar->setRange(mMinVolume/(mMaxVolume/100),100);
}


void VolumeWindow::setBrightIcon(const QString icon)
{
    mIconName = icon;
}

void VolumeWindow::setBrightValue(int value)
{
    mbrightValue = value;
}

void VolumeWindow::timeoutHandle()
{
    hide();
    mTimer->stop();
}

void VolumeWindow::showEvent(QShowEvent* e)
{
     QSize iconSize(ICON_SIZE * mScale,ICON_SIZE * mScale);
    /*适应主题颜色*/
    if(m_styleSettings->get("style-name").toString() == "ukui-light")
    {
        mVolumeBar->setStyleSheet("QProgressBar{border:none;border-radius:3px;background:#33000000}"
                            "QProgressBar::chunk{border-radius:3px;background:black}");
        mBrightBar->setStyleSheet("QProgressBar{border:none;border-radius:3px;background:#33000000}"
                            "QProgressBar::chunk{border-radius:3px;background:black}");
        setPalette(QPalette(QColor("#F5F5F5")));//设置窗口背景色

    }
    else
    {
        mVolumeBar->setStyleSheet("QProgressBar{border:none;border-radius:3px;background:#33000000}"
                            "QProgressBar::chunk{border-radius:3px;background:white}");
        mBrightBar->setStyleSheet("QProgressBar{border:none;border-radius:3px;background:#33000000}"
                            "QProgressBar::chunk{border-radius:3px;background:white}");
        setPalette(QPalette(QColor("#232426")));//设置窗口背景色
    }
    mBut->setPixmap(drawLightColoredPixmap((QIcon::fromTheme(mIconName).pixmap(iconSize)),m_styleSettings->get("style-name").toString()));
}

void VolumeWindow::paintEvent(QPaintEvent* e)
{
    QRect rect = this->rect();
    QPainterPath path;

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // 反锯齿;
    painter.setPen(Qt::transparent);
    qreal radius=12;
    path.moveTo(rect.topRight() - QPointF(radius, 0));
    path.lineTo(rect.topLeft() + QPointF(radius, 0));
    path.quadTo(rect.topLeft(), rect.topLeft() + QPointF(0, radius));
    path.lineTo(rect.bottomLeft() + QPointF(0, -radius));
    path.quadTo(rect.bottomLeft(), rect.bottomLeft() + QPointF(radius, 0));
    path.lineTo(rect.bottomRight() - QPointF(radius, 0));
    path.quadTo(rect.bottomRight(), rect.bottomRight() + QPointF(0, -radius));
    path.lineTo(rect.topRight() + QPointF(0, radius));
    path.quadTo(rect.topRight(), rect.topRight() + QPointF(-radius, -0));

    painter.setBrush(this->palette().base());
    painter.setPen(Qt::transparent);
    painter.setOpacity(0.75);
    painter.drawPath(path);

    KWindowEffects::enableBlurBehind(this->winId(), true, QRegion(path.toFillPolygon().toPolygon()));

    QWidget::paintEvent(e);
}





