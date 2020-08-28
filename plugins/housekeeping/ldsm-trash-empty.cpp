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
#include "ldsm-trash-empty.h"
#include "ui_ldsm-trash-empty.h"

#include <QIcon>
#include <QFile>
#include <QDebug>

#include <glib.h>
#include <QGSettings/qgsettings.h>

LdsmTrashEmpty::LdsmTrashEmpty(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LdsmTrashEmpty)
{
    ui->setupUi(this);
    windowLayoutInit();
}

LdsmTrashEmpty::~LdsmTrashEmpty()
{
    delete ui;
}

void LdsmTrashEmpty::windowLayoutInit()
{
    setFixedSize(400,200);
    setWindowTitle(tr("Emptying the trash"));
    setWindowIcon(QIcon("/new/prefix1/warning.png"));

}
