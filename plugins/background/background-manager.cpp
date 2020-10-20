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
#include "background-manager.h"
#include <QApplication>
#include <X11/Xlib.h>
#include <QX11Info>
#include <X11/Xatom.h>
#include <X11/Xproto.h>
#define BACKGROUND "org.mate.background"
#define PICTURE_FILE_NAME  "picture-filename"
#define COLOR_FILE_NAME    "primary-color"
#define CAN_DRAW_BACKGROUND "draw-background"
BackgroundManager* BackgroundManager::mBackgroundManager = nullptr;

BackgroundManager::BackgroundManager(QScreen *screen, bool is_primary,QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_X11NetWmWindowTypeDesktop);
    resize(QApplication::desktop()->size());
    m_screen = screen;

    initGSettings();
    m_opacity = new QVariantAnimation(this);
    m_opacity->setDuration(50);
    m_opacity->setStartValue(double(0));
    m_opacity->setEndValue(double(1));

    connect(m_opacity, &QVariantAnimation::valueChanged, this, [=]() {
        this->update();
    });
    connect(m_opacity, &QVariantAnimation::finished, this, [=]() {
        m_bg_back_cache_pixmap = m_bg_font_cache_pixmap;
        m_last_pure_color = m_color_to_be_set;
    });
    m_is_primary = is_primary;
    //监听屏幕改变
    connect(m_screen, &QScreen::geometryChanged, this,
            &BackgroundManager::geometryChangedProcess);
    //监听HDMI热插拔
    connect(qApp,SIGNAL(screenAdded(QScreen *)),this,SLOT(screenAddedProcess(QScreen*)));
}

BackgroundManager::~BackgroundManager()
{
    if(settingsNewCreate)
        delete bSettingNew;
    if(settingsOldCreate)
        delete bSettingOld;
}

BackgroundManager* BackgroundManager::getInstance()
{
    if (nullptr == mBackgroundManager) {
        QScreen *screen = QGuiApplication::primaryScreen();
        mBackgroundManager = new BackgroundManager(screen,true);
        mBackgroundManager->show();
    }
    return mBackgroundManager;
}
void BackgroundManager::initGSettings(){
    const QByteArray id(BACKGROUND);
    bSettingOld = new QGSettings(BACKGROUND);
    pFilename = bSettingOld->get(PICTURE_FILE_NAME).toString();
    qFilename = bSettingOld->get(COLOR_FILE_NAME).toString();

    QColor color = qFilename;
    m_last_pure_color = color;
    // 获取更换壁纸前的路径
    m_bg_back_cache_pixmap = QPixmap(pFilename);
    m_bg_font_cache_pixmap = QPixmap(pFilename);
    m_bg_font_pixmap = QPixmap(pFilename);
    m_bg_back_pixmap = m_bg_font_pixmap;
    connect(bSettingOld, SIGNAL(changed(QString)),
            this, SLOT(setup_Background(QString)));

}
void BackgroundManager::setup_Background(const QString &key){
    canDraw = bSettingOld->get(CAN_DRAW_BACKGROUND).toBool();
    if(!canDraw)
        return;
    // 获取更换壁纸后的路径
    pFilename = bSettingOld->get(PICTURE_FILE_NAME).toString();
    qFilename = bSettingOld->get(COLOR_FILE_NAME).toString();
    m_bg_font_pixmap = QPixmap(pFilename);
    m_bg_back_pixmap = m_bg_font_pixmap;
    m_bg_font_cache_pixmap = QPixmap(pFilename);
    m_color_to_be_set = qFilename;

    if (m_opacity->state() == QVariantAnimation::Running) {
        m_opacity->setCurrentTime(20);
    } else {
        m_opacity->stop();
        m_opacity->start();
    }
}

void BackgroundManager::screenAddedProcess(QScreen *screen)
{
    if (screen != nullptr)
        qDebug()<<"screenAdded"<<screen->name()<<screen<<m_window_list.size()<<screen->availableSize();
    else {
        return;
    }

    addWindow(screen, false);
}
void BackgroundManager::addWindow(QScreen *screen, bool checkPrimay)
{
    BackgroundManager *window;
    if (checkPrimay) {
        qDebug()<<"checkPrimay";
        bool is_primary = isPrimaryScreen(screen);
        window = new BackgroundManager(screen, is_primary);
        if (is_primary)
        {
            window->updateView();
        }
    } else {
        window = new BackgroundManager(screen, false);
    }

    m_window_list<<window;
    qDebug()<<"m_window_list"<<m_window_list;
    for (auto window : m_window_list) {
        qDebug()<<"window"<<window;
        window->updateWinGeometry();
    }
}
void BackgroundManager::checkWindowProcess()
{
    //do not check windows, primary window should be handled to exchange in
    //primaryScreenChanged signal emitted.
    return;
    for(auto win : m_window_list)
    {
        //fix duplicate screen cover main screen view problem
        if (win->getScreen() != qApp->primaryScreen())
        {
            if (win->getScreen()->geometry() == qApp->primaryScreen()->geometry())
            {
                win->setVisible(false);
            }
            else {
                win->setVisible(true);
            }
        }
    }
}
bool BackgroundManager::isPrimaryScreen(QScreen *screen)
{
    if (screen == qApp->primaryScreen() && screen)
        return true;

    return false;
}
void BackgroundManager::setScreen(QScreen *screen) {
    m_screen = screen;
}
void BackgroundManager::setIsPrimary(bool is_primary) {
    m_is_primary = is_primary;
}

void BackgroundManager::geometryChangedProcess(const QRect &geometry){
    updateWinGeometry();
}

void BackgroundManager::updateView() {
    auto avaliableGeometry = m_screen->availableGeometry();
    auto geomerty = m_screen->geometry();
    int top = qAbs(avaliableGeometry.top() - geomerty.top());
    int left = qAbs(avaliableGeometry.left() - geomerty.left());
    int bottom = qAbs(avaliableGeometry.bottom() - geomerty.bottom());
    int right = qAbs(avaliableGeometry.right() - geomerty.right());
    //skip unexpected avaliable geometry, it might lead by ukui-panel.
    if (top > 200 | left > 200 | bottom > 200 | right > 200) {
        setContentsMargins(0, 0, 0, 0);
        return;
    }
    setContentsMargins(left, top, right, bottom);
}
void BackgroundManager::scaleBg(const QRect &geometry) {
    if (this->geometry() == geometry)
        return;

    setGeometry(geometry);
    /*!
     * \note
     * There is a bug in kwin, if we directly set window
     * geometry or showFullScreen, window will not be resized
     * correctly.
     *
     * reset the window flags will resovle the problem,
     * but screen will be black a while.
     * this is not user's expected.
     */
    //setWindowFlag(Qt::FramelessWindowHint, false);
//    auto flags = windowFlags() &~Qt::WindowMinMaxButtonsHint;
//    setWindowFlags(flags |Qt::FramelessWindowHint);

    show();

    m_bg_back_cache_pixmap = m_bg_back_pixmap.scaled(geometry.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    m_bg_font_cache_pixmap = m_bg_font_pixmap.scaled(geometry.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    this->update();
}
void BackgroundManager::updateWinGeometry() {
    auto g = getScreen()->geometry();
    //    m_screen = QApplication::primaryScreen();

    scaleBg(g);
    if (m_screen == qApp->primaryScreen()) {
        this->show();
    } else {
        if (m_screen->geometry() == qApp->primaryScreen()->geometry())
            this->hide();
        else
            this->show();
    }
}

void BackgroundManager::paintEvent(QPaintEvent *e){
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    if (m_opacity->state() == QVariantAnimation::Running) {
        if (pFilename == QString::fromLocal8Bit("")) {
            auto opacity = m_opacity->currentValue().toDouble();
            p.fillRect(this->rect(), m_last_pure_color);
            p.save();
            p.setOpacity(opacity);
            p.fillRect(this->rect(), m_color_to_be_set);
            p.restore();
        } else {
            auto opacity = m_opacity->currentValue().toDouble();
            p.drawPixmap(this->rect(), m_bg_back_cache_pixmap, m_bg_back_cache_pixmap.rect());
            p.save();
            p.setOpacity(opacity);
            p.drawPixmap(this->rect(), m_bg_font_cache_pixmap, m_bg_font_cache_pixmap.rect());
            p.restore();
        }
    } else {
        if (pFilename == QString::fromLocal8Bit("")) {
            p.fillRect(this->rect(), m_last_pure_color);
            p.save();
            p.restore();
        } else {
            p.drawPixmap(this->rect(), m_bg_back_cache_pixmap,m_bg_back_cache_pixmap.rect());
            p.save();
            p.restore();
        }
    }
    QWidget::paintEvent(e);
}
