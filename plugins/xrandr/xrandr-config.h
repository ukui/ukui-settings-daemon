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
    std::unique_ptr<xrandrConfig> readFile();
    std::unique_ptr<xrandrConfig> readOpenLidFile();
    bool writeFile();
    bool writeOpenLidFile();

    KScreen::ConfigPtr data() const {
        return mConfig;
    }
    void log();

    void setValidityFlags(KScreen::Config::ValidityFlags flags) {
        mValidityFlags = flags;
    }

    bool canBeApplied() const;

private:
    QString filePath() const;
    std::unique_ptr<xrandrConfig> readFile(const QString &fileName);
    bool writeFile(const QString &filePath);

    bool canBeApplied(KScreen::ConfigPtr config) const;
    static QString configsDirPath();

    KScreen::ConfigPtr mConfig;
    KScreen::Config::ValidityFlags mValidityFlags;

    static QString mConfigsDirName;
    static QString mFixedConfigFileName;


};

#endif // XRANDRCONFIG_H
