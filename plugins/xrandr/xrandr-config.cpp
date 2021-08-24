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
#include <QFile>
#include <QStandardPaths>
#include <QRect>
#include <QJsonDocument>
#include <QDir>

#include <QHBoxLayout>
#include <QtXml>

#include "xrandr-config.h"
#include "clib-syslog.h"

QString xrandrConfig::mFixedConfigFileName = QStringLiteral("fixed-config");
QString xrandrConfig::mConfigsDirName = QStringLiteral("" /*"configs/"*/); // TODO: KDE6 - move these files into the subfolder


QString xrandrConfig::configsDirPath()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) %
            QStringLiteral("/kscreen/");
    return dirPath % mConfigsDirName;
}

QString xrandrConfig::sleepDirPath()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) %
            QStringLiteral("/sleep-state/");
    return dirPath % mConfigsDirName;
}

xrandrConfig::xrandrConfig(KScreen::ConfigPtr config, QObject *parent)
    : QObject(parent)
{
    mConfig = config;
}

QString xrandrConfig::filePath() const
{
    if (!QDir().mkpath(configsDirPath())) {
        return QString();
    }
    return configsDirPath() % id();
}

QString xrandrConfig::id() const
{
    if (!mConfig) {
        return QString();
    }
    return mConfig->connectedOutputsHash();
}

bool xrandrConfig::fileExists() const
{
    return (QFile::exists(configsDirPath() % id()) || QFile::exists(configsDirPath() % mFixedConfigFileName));
}

/*
 * state:是否读取睡眠配置
*/
std::unique_ptr<xrandrConfig> xrandrConfig::readFile(bool state)
{
    bool res = false;
    if (res){//Device::self()->isLaptop() && !Device::self()->isLidClosed()) {
        // We may look for a config that has been set when the lid was closed, Bug: 353029
        const QString lidOpenedFilePath(filePath() % QStringLiteral("_lidOpened"));
        const QFile srcFile(lidOpenedFilePath);

        if (srcFile.exists()) {
            QFile::remove(filePath());
            if (QFile::copy(lidOpenedFilePath, filePath())) {
                QFile::remove(lidOpenedFilePath);
                //qDebug() << "Restored lid opened config to" << id();
            }
        }
    }
    return readFile(id(), state);
}

std::unique_ptr<xrandrConfig> xrandrConfig::readOpenLidFile()
{
    const QString openLidFile = id() % QStringLiteral("_lidOpened");
    auto config = readFile(openLidFile, false);
    QFile::remove(configsDirPath() % openLidFile);
    return config;
}

std::unique_ptr<xrandrConfig> xrandrConfig::readFile(const QString &fileName, bool state)
{
    int enabledOutputsCount = 0;

    if (!mConfig) {
        return nullptr;
    }

    std::unique_ptr<xrandrConfig> config = std::unique_ptr<xrandrConfig>(new xrandrConfig(mConfig->clone()));
    config->setValidityFlags(mValidityFlags);

    QFile file;
    if(!state){
        if (QFile::exists(configsDirPath() % mFixedConfigFileName)) {
            file.setFileName(configsDirPath() % mFixedConfigFileName);
            //qDebug() << "found a fixed config, will use " << file.fileName();
        } else {
            file.setFileName(configsDirPath() % fileName);
        }
        if (!file.open(QIODevice::ReadOnly)) {
            //qDebug() << "failed to open file" << file.fileName();
            return nullptr;
        }
    } else {
        if (QFile::exists(sleepDirPath() % mFixedConfigFileName)) {
            file.setFileName(sleepDirPath() % mFixedConfigFileName);
            //qDebug() << "found a fixed config, will use " << file.fileName();
        } else {
            file.setFileName(sleepDirPath() % fileName);
        }
        if (!file.open(QIODevice::ReadOnly)) {
            //qDebug() << "failed to open file" << file.fileName();
            return nullptr;
        }
    }

    QJsonDocument parser;
    QVariantList outputs = parser.fromJson(file.readAll()).toVariant().toList();
    xrandrOutput::readInOutputs(config->data(), outputs); //不可用

    QSize screenSize;

    for (const auto &output : config->data()->outputs()) {
        USD_LOG_SHOW_OUTPUT(output);
        if (output->isEnabled()) {
            enabledOutputsCount++;
        }

        if (!output->isConnected()) {
            USD_LOG(LOG_DEBUG,"can't positionable..");
            continue;
        }

        if (1 == outputs.count() && (0 != output->pos().x() || 0 != output->pos().y())) {
            USD_LOG(LOG_DEBUG,"read %s at %dx%d size %dx%d",output->name().toLatin1().data(), output->pos().x(), output->pos().y(),
                    output->currentMode()->size().width(),output->currentMode()->size().height());

            const QPoint pos(0,0);
            output->setPos(std::move(pos));
            USD_LOG(LOG_DEBUG,"set %s at %dx%d size %dx%d",output->name().toLatin1().data(), output->pos().x(), output->pos().y(),
                    output->currentMode()->size().width(),output->currentMode()->size().height());
        }

        const QRect geom = output->geometry();
        if (geom.x() + geom.width() > screenSize.width()) {
            USD_LOG(LOG_DEBUG,"x(%d) + width(%d) >screenSize.width(%d)",geom.x(), geom.width(), screenSize.width());
            screenSize.setWidth(geom.x() + geom.width());
        }

        if (geom.y() + geom.height() > screenSize.height()) {
            USD_LOG(LOG_DEBUG,"x(%d) + height(%d) >screenSize.height(%d)",geom.y(), geom.height(), screenSize.height());
            screenSize.setHeight(geom.y() + geom.height());
        }
        USD_LOG_SHOW_OUTPUT(output);
        USD_LOG(LOG_DEBUG,"set %s %dx%d at start at %dx%d by %s, screensize(%dx%d)"
                ,output->name().toLatin1().data(),geom.width(),geom.height(),geom.x(),geom.y(), state ? "sleep":"normal",screenSize.width(),screenSize.height());
    }

    config->data()->screen()->setCurrentSize(screenSize);

    if (!canBeApplied(config->data())) {
        USD_LOG(LOG_DEBUG,"config can't be enabled..cuz maxActiveOutputsCount[%d] : [%d]",
                config->data()->screen()->maxActiveOutputsCount(),enabledOutputsCount);

        config->data()->screen()->setMaxActiveOutputsCount(enabledOutputsCount);

        USD_LOG(LOG_DEBUG,"reset maxActiveOutputCount %d",
                config->data()->screen()->maxActiveOutputsCount());

        if (!canBeApplied(config->data())) {
            USD_LOG(LOG_ERR,"reset maxAcitveOutputsCount but still apply fail.....");
            return nullptr;
        }
    }

    return config;
}

bool xrandrConfig::canBeApplied() const
{
    return canBeApplied(mConfig);
}

bool xrandrConfig::canBeApplied(KScreen::ConfigPtr config) const
{
    return KScreen::Config::canBeApplied(config, mValidityFlags);
}

bool xrandrConfig::writeFile(bool state)
{
    mAddScreen = state;
    return writeFile(filePath(), false);
}

bool xrandrConfig::writeFile(const QString &filePath, bool state)
{
    if (id().isEmpty()) {
        return false;
    }

    const KScreen::OutputList outputs = mConfig->outputs();


    const auto oldConfig = readFile(state);
    KScreen::OutputList oldOutputs;
    if (oldConfig) {
        if(!state)
            return false;
        oldOutputs = oldConfig->data()->outputs();
    }

    QVariantList outputList;
    for (const KScreen::OutputPtr &output : outputs) {
        QVariantMap info;

        const auto oldOutputIt = std::find_if(oldOutputs.constBegin(), oldOutputs.constEnd(),
                                              [output](const KScreen::OutputPtr &out) {
                                                  return out->hashMd5() == output->hashMd5();
                                               }
        );
        const KScreen::OutputPtr oldOutput = oldOutputIt != oldOutputs.constEnd() ? *oldOutputIt :
                                                                                    nullptr;

        if (!output->isConnected()) {
            continue;
        }

        bool priState = false;
        if (state || mAddScreen){
            if (priName.compare(output->name()) == 0){
                priState = true;
            }
        }
        else{
            priState = output->isPrimary();
        }

        xrandrOutput::writeGlobalPart(output, info, oldOutput);
        info[QStringLiteral("primary")] = priState; //
        info[QStringLiteral("enabled")] = output->isEnabled();

        auto setOutputConfigInfo = [&info](const KScreen::OutputPtr &out) {
            if (!out) {
                return;
            }

            QVariantMap pos;
            pos[QStringLiteral("x")] = out->pos().x();
            pos[QStringLiteral("y")] = out->pos().y();
            info[QStringLiteral("pos")] = pos;
        };
        setOutputConfigInfo(output->isEnabled() ? output : oldOutput);

        if (output->isEnabled()) {
            // try to update global output data
            xrandrOutput::writeGlobal(output);
        }

        outputList.append(info);
    }

    if (mAddScreen)
        mAddScreen = false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open config file for writing! " << file.errorString();
        return false;
    }

    file.write(QJsonDocument::fromVariant(outputList).toJson());
    //qDebug() << "Config saved on: " << file.fileName();

    return true;
}

void xrandrConfig::log()
{
    if (!mConfig) {
        return;
    }
    const auto outputs = mConfig->outputs();
    for (const auto &o : outputs) {
        if (o->isConnected()) {
           USD_LOG_SHOW_OUTPUT(o);
        }
    }
}
