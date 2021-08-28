/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * Copyright (C) 2019 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */
#include "widget.h"
#include "kdswidget.h"
#include <QApplication>
#include "qtsingleapplication.h"

#include <X11/Xlib.h>

#include <QTranslator>

int getCurrentScreenWidth(){
    Display * pDis = XOpenDisplay(0);
    if (NULL == pDis){
        return 0;
    }

    Screen * pScreen = DefaultScreenOfDisplay(pDis);
    if (NULL == pScreen){
        return 0;
    }

    int width = pScreen->width;
    XCloseDisplay(pDis);

    return width;
}


int main(int argc, char *argv[])
{
    if (getCurrentScreenWidth() > 2560){
        QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    }

    QString id = QString("kds" + QLatin1String(getenv("DISPLAY")));

    QtSingleApplication app(id, argc, argv);
    if (app.isRunning()){
        app.sendMessage("hello world!");
        return 0; /* EXIT_SUCCESS */
    }

    QTranslator qtTranslator;
    qtTranslator.load(QString(":/%1").arg(QLocale::system().name()));
    app.installTranslator(&qtTranslator);

    KDSWidget kdsw;
    Widget w;

    QString sessionType = qgetenv("XDG_SESSION_TYPE");
    if (QString::compare(sessionType, "wayland") == 0){
        kdsw.beginSetupKF5();
        //    QObject::connect(&app, SIGNAL(messageReceived(const QString&, NULL)), &w, SLOT(msgReceiveAnotherOne(QString)));
        QObject::connect(&app, &QtSingleApplication::messageReceived, &kdsw, &KDSWidget::msgReceiveAnotherOne);
        kdsw.show();

    } else {
        w.beginSetup();
        //    QObject::connect(&app, SIGNAL(messageReceived(const QString&, NULL)), &w, SLOT(msgReceiveAnotherOne(QString)));
        QObject::connect(&app, &QtSingleApplication::messageReceived, &w, &Widget::msgReceiveAnotherOne);
        w.show();
    }

    return app.exec();

}
