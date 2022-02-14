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

#ifndef KDSWIDGET_H
#define KDSWIDGET_H

#include <QWidget>
#include <QButtonGroup>

#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/edid.h>
#include <KF5/KScreen/kscreen/mode.h>
#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/getconfigoperation.h>
#include <KF5/KScreen/kscreen/setconfigoperation.h>

namespace Ui {
class KDSWidget;
}

class KDSWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KDSWidget(QWidget *parent = nullptr);
    ~KDSWidget();

    void beginSetupKF5();

    QPoint getWinPos();
    QString getCurrentPrimaryScreenName();
    int getCurrentScale();

    void setupComponent();
    void setupConnect();

    int getCurrentStatus();

    void setCurrentUIStatus(int id);
    void setCurrentFirstOutputTip();

    void syncPrimaryScreenData(QString pName);

public:
    void setConfig(const KScreen::ConfigPtr &config);
    KScreen::ConfigPtr currentConfig() const;

    void setCloneModeSetup();
    void setExtendModeSetup();
    void setFirstModeSetup();
    void setOtherModeSetup();
    void setLeftExtendModeSetup();

private:
    QString findFirstOutput();
    QSize findBestCloneSize();
    bool turnonSpecifiedOutput(const KScreen::OutputPtr &output, int x, int y);

    int turnonAndGetRightmostOffset(const KScreen::OutputPtr &output, int x);

private:
    Ui::KDSWidget *ui;
    QButtonGroup * btnsGroup;

private:
    KScreen::ConfigPtr mConfig = nullptr;

public Q_SLOTS:
    void msgReceiveAnotherOne(const QString &msg);
    void nextSelectedOption();
    void lastSelectedOption();
    void confirmCurrentOption();
    void receiveButtonClick(int x, int y);
    void closeApp();

Q_SIGNALS:
    void tellBtnClicked(int id);

};

#endif // KDSWIDGET_H
