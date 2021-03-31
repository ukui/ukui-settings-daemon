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
#include <QFont>
#include <QObject>
#include <QPushButton>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>

LdsmTrashEmpty::LdsmTrashEmpty(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LdsmTrashEmpty)
{
    ui->setupUi(this);
    windowLayoutInit();
    connectEvent();
}

LdsmTrashEmpty::~LdsmTrashEmpty()
{
    delete ui;
    delete first_text;
    delete second_text;
    delete trash_empty;
    delete cancel;
}

void LdsmTrashEmpty::windowLayoutInit()
{
    QFont font;
    QDesktopWidget* desktop=QApplication::desktop();
    QRect desk_rect = desktop->screenGeometry(desktop->screenNumber(QCursor::pos()));
    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowMinMaxButtonsHint;
    flags |=Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
    setFixedSize(650,180);
    setWindowTitle(tr("Emptying the trash"));
    setWindowIcon(QIcon::fromTheme("user-trash-full"));
    int dialog_width=width();
    int dialog_height=height();
    int rect_width=desk_rect.width();
    int rect_height=desk_rect.height();
    this->move((rect_width-dialog_width)/2+desk_rect.left(),(rect_height-dialog_height)/2+desk_rect.top());

    first_text=new QLabel(this);
    second_text=new QLabel(this);
    trash_empty=new QPushButton(this);
    cancel=new QPushButton(this);

    first_text->setGeometry(66,20,560,30);
    font.setBold(true);
    first_text->setFont(font);
    first_text->setText(tr("Empty all of the items from the trash?"));
    second_text->setGeometry(66,50,560,30*2);
    second_text->setWordWrap(true);
    second_text->setAlignment(Qt::AlignLeft);
    second_text->setText(tr("If you choose to empty the trash, all items in it will be permanently lost." \
                            "Please note that you can also delete them separately."));
    cancel->setGeometry(dialog_width-140,dialog_height-45,120,30);
    cancel->setText(tr("cancel"));
    trash_empty->setGeometry(dialog_width-270,dialog_height-45,120,30);
    trash_empty->setText(tr("Empty Trash"));
}

void LdsmTrashEmpty::usdLdsmTrashEmpty()
{
    this->exec();
}

void LdsmTrashEmpty::connectEvent()
{
    connect(trash_empty,SIGNAL(clicked()),this,SLOT(checkButtonTrashEmpty()));
    connect(cancel,SIGNAL(clicked()),this,SLOT(checkButtonCancel()));
}

void LdsmTrashEmpty::deleteContents(const QString path)
{
    QDir dir(path);
    QFileInfoList fileList;
    QFileInfo curFile;
    if(!dir.exists())
        return;
    fileList=dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::Readable|QDir::Writable|
                               QDir::Hidden|QDir::NoDotAndDotDot,QDir::Name);
    while(fileList.size()>0)
    {
        int infoNum=fileList.size();
        for(int i=infoNum-1;i>=0;i--)
        {
            curFile=fileList[i];
            if(curFile.isFile())
            {
                QFile fileTemp(curFile.filePath());
                fileTemp.remove();
            }
            if(curFile.isDir())
            {
                QDir dirTemp(curFile.filePath());
                dirTemp.removeRecursively();
            }
            fileList.removeAt(i);
        }
    }
}

void LdsmTrashEmpty::checkButtonTrashEmpty()
{
    QString trash_path;
    trash_path=QDir::homePath()+"/.local/share/Trash";
    deleteContents(trash_path);
    this->accept();
}

void LdsmTrashEmpty::checkButtonCancel()
{
    this->accept();
}
