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
#include <QFile>
#include <QProgressBar>
#include <QPushButton>
#include <QMessageBox>

namespace Ui {
class LdsmTrashEmpty;
}

class LdsmTrashEmpty : public QDialog
{
    Q_OBJECT

public:
    explicit LdsmTrashEmpty(QWidget *parent = nullptr);
    ~LdsmTrashEmpty();
    void trashEmptyStart();
    void showConfirmationDialog();

private:
    Ui::LdsmTrashEmpty *ui;


private:
    void windowLayoutInit();


};

#endif // LDSMTRASHEMPTY_H
