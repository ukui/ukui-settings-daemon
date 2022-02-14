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

#ifndef KEYBOARDWIDGET_H
#define KEYBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QGSettings/qgsettings.h>



namespace Ui {
class KeyboardWidget;
}

class KeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeyboardWidget(QWidget *parent = nullptr);
    ~KeyboardWidget();

    void setIcons(QString icon);
    void showWidget();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent* even);

private:
    void initWidgetInfo();
    void geometryChangedHandle();
    QPixmap drawLightColoredPixmap(const QPixmap &source, const QString &style);
    void repaintWidget();

public Q_SLOTS:
    void timeoutHandle();
    void onStyleChanged(const QString&);

private:
    Ui::KeyboardWidget *ui;
    QWidget*         m_backgroudWidget;
    QString          m_iconName;
    QString             m_LocalIconPath;


    QLabel      *m_btnStatus;
    QTimer           *m_timer;

    QGSettings       *m_styleSettings;
};

#endif // KEYBOARDWIDGET_H
