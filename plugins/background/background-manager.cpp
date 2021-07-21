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

#include <QApplication>
#include <QX11Info>
#include "background-manager.h"
#include <Imlib2.h>
#include <gdk/gdkx.h>

#define BACKGROUND          "org.mate.background"
#define PICTURE_FILE_NAME   "picture-filename"
#define COLOR_FILE_NAME     "primary-color"
#define CAN_DRAW_BACKGROUND "draw-background"


BackgroundManager::BackgroundManager()
{
    m_screen = QApplication::screens().at(0);
}

BackgroundManager::~BackgroundManager()
{
    if(bSettingOld)
        delete bSettingOld;
}

void BackgroundManager::initGSettings(){
    bSettingOld = new QGSettings(BACKGROUND);
    Filename = bSettingOld->get(PICTURE_FILE_NAME).toString();


    connect(bSettingOld, &QGSettings::changed, this, &BackgroundManager::setup_Background);
    connect(qApp, &QApplication::screenAdded, this, &BackgroundManager::screenAddedProcess);
    connect(qApp, &QApplication::screenRemoved, this, &BackgroundManager::screenRemovedProcess);
    connect(m_screen, &QScreen::virtualGeometryChanged, this, &BackgroundManager::virtualGeometryChangedProcess);

}

void BackgroundManager::SetBackground()
{
    Pixmap  pix;
    Window  root;
    Screen  *scn;
    QScreen  *screen;
    Imlib_Image img;
    int ScnNum, width = 0,height = 0;

    dpy = gdk_x11_get_default_xdisplay();

    img = imlib_load_image(Filename.toLatin1().data());
    if (!img) {
        USD_LOG(LOG_DEBUG,"%s:Unable to load image\n", Filename.toLatin1().data());
        return ;
    }
    imlib_context_set_image(img);

    if (!dpy)
        return ;

    ScnNum = QApplication::screens().length();

    width = DisplayWidth(dpy, 0);
    height = DisplayHeight(dpy, 0);

    scn = DefaultScreenOfDisplay(dpy);
    root = DefaultRootWindow(dpy);

    pix = XCreatePixmap(dpy, root, width, height,
                        DefaultDepthOfScreen(scn));

    imlib_context_set_display(dpy);
    imlib_context_set_visual(DefaultVisualOfScreen(scn));
    imlib_context_set_colormap(DefaultColormapOfScreen(scn));
    imlib_context_set_drawable(pix);
    ScnNum = QApplication::screens().length();

    for(int i = 0; i < ScnNum; i++){
        screen =  QApplication::screens().at(i);
        //qDebug()<<screen->geometry();
        imlib_render_image_on_drawable_at_size(screen->geometry().x() * 2,
                                               screen->geometry().y() * 2,
                                               screen->geometry().width() *2,
                                               screen->geometry().height() * 2);
    }

    XSetWindowBackgroundPixmap(dpy, root, pix);
    XClearWindow(dpy, root);

    while (XPending(dpy)) {
        XEvent ev;
        XNextEvent(dpy, &ev);
    }

    XFreePixmap(dpy, pix);
    imlib_free_image();
}

void BackgroundManager::setup_Background(const QString &key)
{
    if(key.compare(QString::fromLocal8Bit(PICTURE_FILE_NAME))==0)
        Filename = bSettingOld->get(PICTURE_FILE_NAME).toString();
    if(key.compare(QString::fromLocal8Bit(COLOR_FILE_NAME))==0)
        Filename = bSettingOld->get(COLOR_FILE_NAME).toString();
    SetBackground();
}

void BackgroundManager::screenAddedProcess(QScreen *screen)
{
    SetBackground();
}

void BackgroundManager::screenRemovedProcess(QScreen *screen)
{
    SetBackground();
}

void BackgroundManager::virtualGeometryChangedProcess(const QRect &geometry)
{
     SetBackground();
}
void BackgroundManager::BackgroundManagerStart()
{
    initGSettings();
    //SetBackground();
}
