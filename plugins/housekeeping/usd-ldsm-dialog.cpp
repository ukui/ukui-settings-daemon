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
#include "usd-ldsm-dialog.h"
#include "ui_usd-ldsm-dialog.h"

#include <QDesktopWidget>
#include <QRect>
#include <QImage>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QFont>
#include <QList>
#include <QDebug>

#include <QGSettings/qgsettings.h>
#include <syslog.h>
#include <glib.h>
#include "clib-syslog.h"

LdsmDialog::LdsmDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LdsmDialog)
{
    ui->setupUi(this);
}
LdsmDialog::LdsmDialog(bool other_usable_partitions,bool other_partitions,bool display_baobab,bool has_trash,
                       long space_remaining,QString partition_name,QString mount_path,
                       QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::LdsmDialog)
{
    ui->setupUi(this);
    this->other_usable_partitions=other_usable_partitions;
    this->other_partitions=other_partitions;
    this->has_trash=has_trash;
    this->space_remaining=space_remaining;
    this->partition_name=partition_name;
    this->mount_path=mount_path;
    this->analyze_button=nullptr;
    windowLayoutInit(display_baobab);
    allConnectEvent(display_baobab);
}

LdsmDialog::~LdsmDialog()
{
    delete ui;
    delete picture_label;
    delete primary_label;
    delete second_label;
    delete ignore_check_button;
    delete ignore_button;

    if(this->has_trash)
        delete trash_empty;
    if(analyze_button)
        delete analyze_button;
}

void LdsmDialog::windowLayoutInit(bool display_baobab)
{
    QFont font;
    QDesktopWidget* desktop=QApplication::desktop();
    QRect desk_rect = desktop->screenGeometry(desktop->screenNumber(QCursor::pos()));
    Qt::WindowFlags flags=Qt::Dialog;
    flags |=Qt::WindowMinMaxButtonsHint;
    flags |=Qt::WindowCloseButtonHint;
    setWindowFlags(flags);
    setFixedSize(660,210);
    setWindowIcon(QIcon::fromTheme("dialog-warning"));
    int dialog_width=width();
    int dialog_height=height();
    int rect_width=desk_rect.width();
    int rect_height=desk_rect.height();

    setWindowTitle(tr("Low Disk Space"));
    this->move((rect_width-dialog_width)/2+desk_rect.left(),(rect_height-dialog_height)/2+desk_rect.top());

    picture_label=new QLabel(this);
    primary_label=new QLabel(this);
    second_label=new QLabel(this);
    ignore_check_button=new QCheckBox(this);

    ignore_button=new QPushButton(this);
    //warning picture
    picture_label->setGeometry(20,40,32,32);
    picture_label->setAlignment(Qt::AlignCenter);
    picture_label->setStyleSheet(QString("border-image:url(../ldsm_dialog/warning.png);"));
    //warning information text
    primary_label->setGeometry(66,20,560,30);
    second_label->setGeometry(66,50,560,30*2);
    second_label->setWordWrap(true);
    second_label->setAlignment(Qt::AlignLeft);

    font.setBold(true);
    primary_label->setFont(font);
    primary_label->setText(getPrimaryText());
    second_label->setText(getSecondText());
    //gsettings set box
    ignore_check_button->setGeometry(70,135,400,30);
    ignore_check_button->setText(getCheckButtonText());

    ignore_button->setGeometry(dialog_width-110,dialog_height-40,100,30);
    ignore_button->setText(tr("Ignore"));

    if(this->has_trash){
        trash_empty = new QPushButton(this);
        trash_empty->setGeometry(dialog_width-240,dialog_height-40,120,30);
        trash_empty->setText(tr("Empty Trash"));
    }
    if(display_baobab){
        analyze_button = new QPushButton(this);
        analyze_button->setText(tr("Examine"));
        if(this->has_trash)
            analyze_button->setGeometry(dialog_width-320,dialog_height-40,100,30);
        else
            analyze_button->setGeometry(dialog_width-215,dialog_height-40,100,30);
    }
}

QString LdsmDialog::getPrimaryText()
{
    QString primary_text;
    char* free_space=g_format_size(space_remaining);
    if(other_partitions)
        primary_text=QString().sprintf((tr("The volume \"%1\" has only %s disk space remaining.")).toLocal8Bit().constData(),
                                       free_space).arg(partition_name);
    else
        primary_text=QString().sprintf((tr("The computer has only %s disk space remaining.")).toLocal8Bit().constData(),
                                       free_space);
    return primary_text;
}

QString LdsmDialog::getSecondText()
{
    if(other_usable_partitions){
        if(has_trash)
            return QString(tr("You can free up disk space by emptying the Trash, removing " \
                           "unused programs or files, or moving files to another disk or partition."));
        else
            return QString(tr("You can free up disk space by removing unused programs or files, " \
                           "or by moving files to another disk or partition."));
    }else{
        if(has_trash)
            return QString(tr("You can free up disk space by emptying the Trash, removing unused " \
                           "programs or files, or moving files to an external disk."));
        else
            return QString(tr("You can free up disk space by removing unused programs or files, " \
                           "or by moving files to an external disk."));
    }
}

QString LdsmDialog::getCheckButtonText()
{
    if(other_partitions)
        return QString(tr("Don't show any warnings again for this file system"));
    else
        return QString(tr("Don't show any warnings again"));
}

void LdsmDialog::allConnectEvent(bool display_baobab)
{
    connect(ignore_check_button, &QCheckBox::stateChanged,
            this,   &LdsmDialog::checkButtonClicked);

    connect(ignore_button, &QPushButton::clicked,
            this,   &LdsmDialog::checkButtonIgnore);

    if(has_trash)
        connect(trash_empty, &QPushButton::clicked,
                this,   &LdsmDialog::checkButtonTrashEmpty);

    if(display_baobab)
        connect(analyze_button, &QPushButton::clicked,
                this, &LdsmDialog::checkButtonAnalyze);

    if(sender() == ignore_button) {
        USD_LOG(LOG_DEBUG,"Ignore button pressed!");
    } else {
        USD_LOG(LOG_DEBUG,"Other button pressed!");
    }
}

//update gsettings "ignore-paths" key contents
bool update_ignore_paths(QList<QString>** ignore_paths,QString mount_path,bool ignore)
{
    bool found=(*ignore_paths)->contains(mount_path.toLatin1().data());
    if(ignore && found==false){
        (*ignore_paths)->push_front(mount_path.toLatin1().data());
        return true;
    }
    if(!ignore && found==true){
        (*ignore_paths)->removeOne(mount_path.toLatin1().data());
        return true;
    }
    return false;
}

/****************slots*********************/
void LdsmDialog::checkButtonIgnore()
{
    done(LDSM_DIALOG_IGNORE);
}

void LdsmDialog::checkButtonTrashEmpty()
{
    done(LDSM_DIALOG_RESPONSE_EMPTY_TRASH);
}

void LdsmDialog::checkButtonAnalyze()
{
    done(LDSM_DIALOG_RESPONSE_ANALYZE);
}

void LdsmDialog::checkButtonClicked(int state)
{
    QGSettings* settings;
    QStringList ignore_list;
    QStringList ignoreStr;
    QList<QString>* ignore_paths;
    bool ignore,updated;
    int i;
    QList<QString>::iterator l;

    ignore_paths =new QList<QString>();
    settings = new QGSettings(SETTINGS_SCHEMA);
    //get contents from "ignore-paths" key
    if (!settings->get(SETTINGS_IGNORE_PATHS).toStringList().isEmpty())
        ignore_list.append(settings->get(SETTINGS_IGNORE_PATHS).toStringList());

    for(auto str: ignore_list){
        if(!str.isEmpty())
            ignore_paths->push_back(str);
    }

    ignore = state;
    updated = update_ignore_paths(&ignore_paths, mount_path, ignore);

    if(updated){
        for(l = ignore_paths->begin(); l != ignore_paths->end(); ++l){
            ignoreStr.append(*l);
        }
        //set latest contents to gsettings "ignore-paths" key
        settings->set(SETTINGS_IGNORE_PATHS, QVariant::fromValue(ignoreStr));
    }
    //free QList Memory
    if(ignore_paths){
        ignore_paths->clear();
    }
    delete settings;
}
