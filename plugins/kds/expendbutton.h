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

#ifndef EXPENDBUTTON_H
#define EXPENDBUTTON_H


#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QGSettings/QGSettings>

class ExpendButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ExpendButton(QWidget *parent = 0);
    ~ExpendButton();

public:
    void setSign(int id ,const QString &style = "");
    void setBtnLogo(QString logo,const QString &style = "");
    void setBtnText(QString text);
    void setBtnChecked(bool checked);
    bool getBtnChecked();

private:
    QPixmap drawLightColoredPixmap(const QPixmap &source, const QString &style);


private:
    QLabel * logoLabel;
    QLabel * textLabel;
    QLabel * spaceLabel;
    QLabel * statusLabel;
    QGSettings     *m_styleSettings;


    int sign;

    QString qss0;
    QString qss1;

};

#endif // EXPENDBUTTON_H
