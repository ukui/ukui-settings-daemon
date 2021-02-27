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
#ifndef OUTPUT_H
#define OUTPUT_H

#include <KF5/KScreen/kscreen/types.h>
#include <QOrientationReading>
#include <QVariantMap>

class xrandrOutput
{
public:
    static void readInOutputs(KScreen::ConfigPtr config,
                              const QVariantList &outputsInfo);

    static void writeGlobal(const KScreen::OutputPtr &output);
    static bool writeGlobalPart(const KScreen::OutputPtr &output,
                                QVariantMap &info,
                                const KScreen::OutputPtr &fallback);

    static QString dirPath();

private:
    static QString globalFileName(const QString &hash);
    static QVariantMap getGlobalData(KScreen::OutputPtr output);

    static void readIn(KScreen::OutputPtr output, const QVariantMap &info);
    static bool readInGlobal(KScreen::OutputPtr output);
    static void readInGlobalPartFromInfo(KScreen::OutputPtr output, const QVariantMap &info);
    /*
     * When a global output value (scale, rotation) is changed we might
     * need to reposition the outputs when another config is read.
     */
    static void adjustPositions(KScreen::ConfigPtr config, const QVariantList &outputsInfo);

    static QString mDirName;
};

#endif // OUTPUT_H
