/* -*- Mode: C++; indent-tabs-mode: nil; tab-width: 4 -*-
 * -*- coding: utf-8 -*-
 *
 * Copyright (C) 2012 by Alejandro Fiestas Olivares <afiestas@kde.org>
 * Copyright 2016 by Sebastian Kügler <sebas@kde.org>
 * Copyright (c) 2018 Kai Uwe Broulik <kde@broulik.de>
 *                    Work sponsored by the LiMux project of
 *                    the city of Munich.
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
#ifndef XRANDRCONFIG_H
#define XRANDRCONFIG_H

#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/configmonitor.h>

#include <QOrientationReading>
#include <memory>
#include "xrandr-output.h"


class xrandrConfig : public QObject
{
    Q_OBJECT
public:
    explicit xrandrConfig(KScreen::ConfigPtr config, QObject *parent = nullptr);
    ~xrandrConfig() = default;

    QString id() const;

    bool fileExists() const;
    bool fileScreenModeExists(QString screenMode);

    std::unique_ptr<xrandrConfig> readFile(bool isUseModeConfig);
    std::unique_ptr<xrandrConfig> readOpenLidFile();
    std::unique_ptr<xrandrConfig> readFile(const QString &fileName, bool state);

    bool writeFile(bool state);
    bool writeOpenLidFile();
    bool writeConfigAndBackupToModeDir();
    bool writeFile(const QString &filePath, bool state);
    QString getScreensParam();
    KScreen::ConfigPtr data() const {
        return mConfig;
    }
    void log();

    void setPriName(QString name){
        priName = name;
    }
    void setValidityFlags(KScreen::Config::ValidityFlags flags) {
        mValidityFlags = flags;
    }

    bool canBeApplied() const;

    QString filePath() const;
    QString fileModeConfigPath();

    void setScreenMode(QString modeName);
private:


    bool canBeApplied(KScreen::ConfigPtr config) const;
    static QString configsDirPath();
    QString configsModeDirPath();
    static QString sleepDirPath();


    KScreen::ConfigPtr mConfig;
    KScreen::Config::ValidityFlags mValidityFlags;

    QString priName;
    bool    mAddScreen = false;

    QString mScreenMode;
    static QString mConfigsDirName;
    static QString mFixedConfigFileName;


};

#endif // XRANDRCONFIG_H
