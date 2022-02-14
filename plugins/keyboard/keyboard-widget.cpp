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

#include "keyboard-widget.h"
#include "ui_keyboardwidget.h"
#include <QTimer>
#include <QPainter>
#include <QBitmap>
#include <QScreen>
#include "clib-syslog.h"
#include <QDebug>
#include <QLayout>
#include <QPixmap>
#include <QIcon>
#include <KWindowEffects>
#include "usd_base_class.h"

#define QT_THEME_SCHEMA             "org.ukui.style"

#define PANEL_SCHEMA "org.ukui.panel.settings"
#define PANEL_SIZE_KEY "panelsize"

#define DEFAULT_LOCALE_ICON_NAME ":/ukui_res/ukui/"
#define INTEL_LOCALE_ICON_NAME ":/ukui_res/ukui_intel/"

KeyboardWidget::KeyboardWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KeyboardWidget)
{
    ui->setupUi(this);
    initWidgetInfo();
}

KeyboardWidget::~KeyboardWidget()
{
    delete ui;
}

void KeyboardWidget::initWidgetInfo()
{
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint |
                   Qt::X11BypassWindowManagerHint |
                   Qt::Popup);

    setFixedSize(72,72);
    setAttribute(Qt::WA_TranslucentBackground, true);

    if(UsdBaseClass::isTablet()) {
        m_LocalIconPath = INTEL_LOCALE_ICON_NAME;
    } else {
        m_LocalIconPath = DEFAULT_LOCALE_ICON_NAME;
    }

    m_styleSettings = new QGSettings(QT_THEME_SCHEMA);
    connect(m_styleSettings,SIGNAL(changed(const QString&)),
            this,SLOT(onStyleChanged(const QString&)));


    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));

    connect(QApplication::primaryScreen(), &QScreen::geometryChanged, this, &KeyboardWidget::geometryChangedHandle);

    connect(static_cast<QApplication *>(QCoreApplication::instance()),
            &QApplication::primaryScreenChanged, this, &KeyboardWidget::geometryChangedHandle);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(0);

    m_btnStatus = new QLabel(this);
    m_btnStatus->setFixedSize(QSize(48,48));
    layout->addWidget(m_btnStatus, 0, Qt::AlignTop | Qt::AlignHCenter);

    setAutoFillBackground(true);
    geometryChangedHandle();

}


void KeyboardWidget::timeoutHandle()
{
    m_timer->stop();
    hide();
}
void KeyboardWidget::showWidget()
{
    geometryChangedHandle();
    repaintWidget();
    show();
    m_timer->start(2500);

}


void KeyboardWidget::setIcons(QString icon)
{
    m_iconName = icon;
}

void KeyboardWidget::geometryChangedHandle()
{
    int x=QApplication::primaryScreen()->geometry().x();
    int y=QApplication::primaryScreen()->geometry().y();
    int width = QApplication::primaryScreen()->size().width();
    int height = QApplication::primaryScreen()->size().height();

    int pSize = 0;
    const QByteArray id(PANEL_SCHEMA);
    if (QGSettings::isSchemaInstalled(id)){
        QGSettings * settings = new QGSettings(id);
        pSize = settings->get(PANEL_SIZE_KEY).toInt();

        delete settings;
    }
    int ax,ay;
    ax = x+width - this->width() - 200;
    ay = y+height - this->height() - pSize-4;
    move(ax,ay);
}

void KeyboardWidget::onStyleChanged(const QString& key)
{
    Q_UNUSED(key)
    if(!this->isHidden()) {
        hide();
        repaintWidget();
        show();
    }
}

QPixmap KeyboardWidget::drawLightColoredPixmap(const QPixmap &source, const QString &style)
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

void KeyboardWidget::repaintWidget()
{
    if(m_styleSettings->get("style-name").toString() == "ukui-light"){
        setPalette(QPalette(QColor("#F5F5F5")));//设置窗口背景
    } else{
        setPalette(QPalette(QColor("#232426")));//设置窗口背景色
    }
    QString m_LocalIconName;
    m_LocalIconName = m_LocalIconPath + m_iconName + QString(".svg");
    QPixmap m_pixmap = QIcon::fromTheme(m_iconName,QIcon(m_LocalIconName)).pixmap(QSize(48,48));
    m_btnStatus->setPixmap(drawLightColoredPixmap(m_pixmap,m_styleSettings->get("style-name").toString()));
}


void KeyboardWidget::resizeEvent(QResizeEvent* event)
{
    m_btnStatus->move((width() - m_btnStatus->width())/2,(height() - m_btnStatus->height())/2);
    QWidget::resizeEvent(event);
}

void KeyboardWidget::paintEvent(QPaintEvent *event)
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


