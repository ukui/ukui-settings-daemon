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
#ifndef LDSMTRASHEMPTY_H
#define LDSMTRASHEMPTY_H

#include <QObject>
#include <QDialog>
#include <QLabel>
#include <QPushButton>

namespace Ui {
class LdsmTrashEmpty;
}

class LdsmTrashEmpty : public QDialog
{
    Q_OBJECT

public:
    explicit LdsmTrashEmpty(QWidget *parent = nullptr);
    ~LdsmTrashEmpty();
    void usdLdsmTrashEmpty();

public Q_SLOTS:
    void checkButtonCancel();
    void checkButtonTrashEmpty();

private:
    Ui::LdsmTrashEmpty *ui;
    QLabel *first_text;
    QLabel *second_text;
    QPushButton *trash_empty;
    QPushButton *cancel;

private:
    void windowLayoutInit();
    void connectEvent();
    void deleteContents(const QString path);

};

#endif // LDSMTRASHEMPTY_H
