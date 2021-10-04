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
#include <KWindowEffects>
#include "clib-syslog.h"
#define DBUS_NAME       "org.ukui.SettingsDaemon"
#define DBUS_PATH       "/org/ukui/SettingsDaemon/wayland"
#define DBUS_INTERFACE  "org.ukui.SettingsDaemon.wayland"

#define QT_THEME_SCHEMA             "org.ukui.style"

#define PANEL_SCHEMA "org.ukui.panel.settings"
#define PANEL_SIZE_KEY "panelsize"

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

    m_styleSettings = new QGSettings(QT_THEME_SCHEMA);
    connect(m_styleSettings,SIGNAL(changed(const QString&)),
            this,SLOT(onStyleChanged(const QString&)));

    mScale = getScreenGeometry("scale");
}

DeviceWindow::~DeviceWindow()
{
    delete ui;
    delete mTimer;
    mTimer = nullptr;
}

/* 主屏幕变化监听函数 */
void DeviceWindow::priScreenChanged(int x, int y, int Width, int Height)
{
    const QByteArray id(PANEL_SCHEMA);

    int pSize = 0;

    if (QGSettings::isSchemaInstalled(id)){
        QGSettings * settings = new QGSettings(id);
        pSize = settings->get(PANEL_SIZE_KEY).toInt();

        delete settings;
    }

    int ax,ay;
    ax = x+Width - this->width() - 200;
    ay = y+Height - this->height() - pSize-4;
    move(ax,ay);

    USD_LOG(LOG_DEBUG,"move it at %d,%d",ax,ay);

}
void DeviceWindow::geometryChangedHandle()
{
    int x=QApplication::primaryScreen()->geometry().x();
    int y=QApplication::primaryScreen()->geometry().y();
    int width = QApplication::primaryScreen()->size().width();
    int height = QApplication::primaryScreen()->size().height();

    USD_LOG(LOG_DEBUG,"getchangehandle....%dx%d at(%d,%d)",width,height,x,y);
    priScreenChanged(x,y,width,height);
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
    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));

    m_btnStatus = new QLabel(this);
    m_btnStatus->setFixedSize(QSize(48,48));


    connect(QApplication::primaryScreen(), &QScreen::geometryChanged, this, &DeviceWindow::geometryChangedHandle);
    connect(static_cast<QApplication *>(QCoreApplication::instance()),
            &QApplication::primaryScreenChanged, this, &DeviceWindow::geometryChangedHandle);


    setFixedSize(72,72);
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint |
                   Qt::X11BypassWindowManagerHint |
                   Qt::Popup);
    setAttribute(Qt::WA_TranslucentBackground, true);


    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#232426"));
    p.setRenderHint(QPainter::Antialiasing);
    p.drawRoundedRect(bmp.rect(),12,12);
    setMask(bmp);

    QPainterPath path;
    auto rect = this->rect();
    rect.adjust(1, 1, -1, -1);
    path.addRect(rect);
    KWindowEffects::enableBlurBehind(this->winId(), true, QRegion(path.toFillPolygon().toPolygon()));


    if(m_styleSettings->get("style-name").toString() == "ukui-light")
    {
        setPalette(QPalette(QColor("#F5F5F5")));//设置窗口背景色

    }
    else
    {
        setPalette(QPalette(QColor("#232426")));//设置窗口背景色

    }
    setAutoFillBackground(true);

    geometryChangedHandle();
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
    geometryChangedHandle();
    int svgWidth,svgHeight;
    int svgX,svgY;

    ensureSvgInfo(&svgWidth,&svgHeight,&svgX,&svgY);

    m_btnStatus->move((width() - m_btnStatus->width())/2,(height() - m_btnStatus->height())/2);

    QPixmap pixmap = QIcon::fromTheme(mIconName,QIcon("")).pixmap(QSize(48,48));
    m_btnStatus->setPixmap(drawLightColoredPixmap(pixmap,m_styleSettings->get("style-name").toString()));

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

QPixmap DeviceWindow::drawLightColoredPixmap(const QPixmap &source, const QString &style)
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
void DeviceWindow::onStyleChanged(const QString&)
{
    if(m_styleSettings->get("style-name").toString() == "ukui-light")
    {
        setPalette(QPalette(QColor("#F5F5F5")));//设置窗口背景色

    }
    else
    {
        setPalette(QPalette(QColor("#232426")));//设置窗口背景色

    }
}

void DeviceWindow::paintEvent(QPaintEvent *event)
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

    QWidget::paintEvent(event);
}

