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

#include "a11y-preferences-dialog.h"
#include "ui_a11y-preferences-dialog.h"

A11yPreferencesDialog::A11yPreferencesDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::A11yPreferencesDialog)
{
    ui->setupUi(this);
}

A11yPreferencesDialog::~A11yPreferencesDialog()
{
    delete ui;
}

void A11yPreferencesDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    USD_LOG(LOG_DEBUG,"dialog had close");
    Q_EMIT singalCloseWidget();
}
