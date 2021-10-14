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
#ifndef DEVICEWINDOW_H
#define DEVICEWINDOW_H

#include <QWidget>
#include <QString>
#include <QSvgWidget>
#include <QApplication>
#include <QX11Info>
#include <QScreen>
#include <QTimer>
#include <QAction>
#include <QLabel>
#include <QPushButton>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusInterface>
#include <QGSettings/qgsettings.h>

namespace Ui {
class DeviceWindow;
}

class DeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceWindow(QWidget *parent = nullptr);
    ~DeviceWindow();
    void initWindowInfo();
    void setAction(const QString);
    void dialogShow();
    int getScreenGeometry (QString methodName);

private:
    QPixmap drawLightColoredPixmap(const QPixmap &source, const QString &style);


private Q_SLOTS:
    void timeoutHandle();
    void priScreenChanged(int x, int y, int width, int height);
    void geometryChangedHandle();
    void repaintWidget();


    void onStyleChanged(const QString& );
protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent* event);



private:
    Ui::DeviceWindow *ui;
    double           mScale = 1;
    QString          mIconName;
    QString          m_LocalIconPath;
    QLabel           *m_btnStatus;
    QTimer           *mTimer;
    QDBusInterface   *mDbusXrandInter;

    QGSettings       *m_styleSettings;

};

#endif // DEVICEWINDOW_H
