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
#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QKeyEvent>
#include <QButtonGroup>
#include <QGSettings/QGSettings>
#include "usd_base_class.h"

#ifdef signals
#undef signals
#endif

extern "C" {
#define MATE_DESKTOP_USE_UNSTABLE_API
#include <libmate-desktop/mate-rr.h>
#include <libmate-desktop/mate-rr-config.h>
#include <libmate-desktop/mate-rr-labeler.h>

#include <gtk/gtk.h>
}

class QDBusInterface;


namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = 0);
    ~Widget();

public:
    void beginSetup();

    void initData();
    void setupComponent();
    void setupConnect();

    void setScreenModeByDbus(QString modeName);
    int getCurrentStatus();
    QString getScreensParam();
    void initCurrentStatus(int id);

private:
    Ui::Widget *ui;
    QButtonGroup *btnsGroup;
    QMetaEnum metaEnum;
private:
    MateRRScreen * kScreen;
//    MateRRConfig * kConfig;

    bool           m_superPresss;

    QGSettings     *m_scaleSetting;
    QGSettings     *m_styleSettings;

    QString         m_qssDefaultOrDark;
    QString         m_qssLight;

    double         m_scale;

protected:
    void keyPressEvent(QKeyEvent *event);
    void paintEvent(QPaintEvent *event);
    void showEvent(QShowEvent* event);

public Q_SLOTS:
    void XkbButtonEvent(int,int);
    void msgReceiveAnotherOne(const QString &msg);
    void geometryChangedHandle();
    void screensParamChangedSignal(QString screensParam);

private:

private Q_SLOTS:
    void nextSelectedOption();
    void lastSelectedOption();
    void confirmCurrentOption();
    void receiveButtonClick(int x, int y);
    void closeApp();

Q_SIGNALS:
    void tellBtnClicked(int id);

};

#endif // WIDGET_H

