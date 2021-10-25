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

void xrandrConfig::setScreenMode(QString modeName)
{
    mScreenMode = modeName;
    USD_LOG(LOG_DEBUG,"set mScreenMode to :%s",mScreenMode.toLatin1().data());
}

QString xrandrConfig::configsModeDirPath()
{
    QString dirPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) %
            QStringLiteral("/kscreen/") % mScreenMode % QStringLiteral("/");
    return dirPath;
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

QString xrandrConfig::fileModeConfigPath()
{
    if (!QDir().mkpath(configsModeDirPath())) {
        return QString();
    }
    return configsModeDirPath() % id();
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

bool xrandrConfig::fileScreenModeExists(QString screenMode)
{
    USD_LOG(LOG_DEBUG,"%s status:%d",(fileModeConfigPath()).toLatin1().data(),QFile::exists(fileModeConfigPath()));
    return QFile::exists(fileModeConfigPath());
}

/*
 * isUseModeConfig:是否读取模式配置
 * 模式配置只是在kds调用接口时使用
*/
std::unique_ptr<xrandrConfig> xrandrConfig::readFile(bool isUseModeConfig)
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
    return readFile(id(), isUseModeConfig);
}

std::unique_ptr<xrandrConfig> xrandrConfig::readOpenLidFile()
{
    const QString openLidFile = id() % QStringLiteral("_lidOpened");
    auto config = readFile(openLidFile, false);
    QFile::remove(configsDirPath() % openLidFile);
    return config;
}

std::unique_ptr<xrandrConfig> xrandrConfig::readScreensConfigFromDbus(const QString &screensParam)
{
    std::unique_ptr<xrandrConfig> config = std::unique_ptr<xrandrConfig>(new xrandrConfig(mConfig->clone()));
    config->setValidityFlags(mValidityFlags);

    QJsonDocument parser;
    QVariantList outputs = parser.fromJson(screensParam.toLatin1().data()).toVariant().toList();
    xrandrOutput::readInOutputs(config->data(), outputs);
    QSize screenSize;

    for (const auto &output : config->data()->outputs()) {
        if (!output->isConnected()) {
            continue;
        }

        if (1 == outputs.count() && (0 != output->pos().x() || 0 != output->pos().y())) {
            const QPoint pos(0,0);
            output->setPos(std::move(pos));
        }

        const QRect geom = output->geometry();
        if (geom.x() + geom.width() > screenSize.width()) {
            screenSize.setWidth(geom.x() + geom.width());
        }

        if (geom.y() + geom.height() > screenSize.height()) {
            screenSize.setHeight(geom.y() + geom.height());
        }
    }

    if (!canBeApplied(config->data())) {
        USD_LOG(LOG_ERR,"is a error param form dbus..");
        return nullptr;
    }

    return config;
}

std::unique_ptr<xrandrConfig> xrandrConfig::readFile(const QString &fileName, bool state)
{
    int enabledOutputsCount = 0;

    if (!mConfig) {
        USD_LOG(LOG_ERR,"config is nullptr...");
        return nullptr;
    }

    std::unique_ptr<xrandrConfig> config = std::unique_ptr<xrandrConfig>(new xrandrConfig(mConfig->clone()));
    config->setValidityFlags(mValidityFlags);

    QFile file;
    if(!state){
        if (QFile::exists(configsDirPath() % mFixedConfigFileName)) {
            file.setFileName(configsDirPath() % mFixedConfigFileName);//先读取特定模式的配置，
        } else {
            file.setFileName(configsDirPath() % fileName);
        }
        if (!file.open(QIODevice::ReadOnly)) {
            USD_LOG(LOG_ERR,"config is nullptr...");
            return nullptr;
        }

    } else {
        if (QFile::exists(configsModeDirPath())) {
            file.setFileName(configsModeDirPath() % fileName);
        }

        if (!file.open(QIODevice::ReadOnly)) {
             USD_LOG(LOG_ERR,"config is nullptr...%s",file.fileName().toLatin1().data());
            return nullptr;
        }
    }
    QJsonDocument parser;
    QVariantList outputs = parser.fromJson(file.readAll()).toVariant().toList();
    xrandrOutput::readInOutputs(config->data(), outputs);

    QSize screenSize;

    for (const auto &output : config->data()->outputs()) {
        if (output->isEnabled()) {
            enabledOutputsCount++;
        }

        if (!output->isConnected()) {
            continue;
        }

        if (1 == outputs.count() && (0 != output->pos().x() || 0 != output->pos().y())) {
            const QPoint pos(0,0);
            output->setPos(std::move(pos));
        }

        const QRect geom = output->geometry();
        if (geom.x() + geom.width() > screenSize.width()) {
            screenSize.setWidth(geom.x() + geom.width());
        }

        if (geom.y() + geom.height() > screenSize.height()) {
            screenSize.setHeight(geom.y() + geom.height());
        }
      }

    config->data()->screen()->setCurrentSize(screenSize);

    if (!canBeApplied(config->data())) {
        config->data()->screen()->setMaxActiveOutputsCount(enabledOutputsCount);

        if (!canBeApplied(config->data())) {
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

bool xrandrConfig::writeConfigAndBackupToModeDir()
{
    return true;
}

QString xrandrConfig::getScreensParam()
{
    const KScreen::OutputList outputs = mConfig->outputs();

    QVariantList outputList;
    for (const KScreen::OutputPtr &output : outputs) {
        QVariantMap info;

        if (false == output->isConnected()) {
            continue;
        }

        xrandrOutput::writeGlobalPart(output, info, nullptr);
        info[QStringLiteral("primary")] =  output->isPrimary();; //
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
        setOutputConfigInfo(output->isEnabled() ? output : nullptr);

        outputList.append(info);
    }

    return QJsonDocument::fromVariant(outputList).toJson();
}

bool xrandrConfig::writeFile(const QString &filePath, bool state)
{
    int screenConnectedCount = 0;
    if (id().isEmpty()) {
        USD_LOG(LOG_DEBUG,"id is empty!");
        return false;
    }

    const KScreen::OutputList outputs = mConfig->outputs();

    QVariantList outputList;
    for (const KScreen::OutputPtr &output : outputs) {
        QVariantMap info;

        if (!output->isConnected()) {
            continue;
        }
        screenConnectedCount++;
        bool priState = false;
        if (state || mAddScreen){
            if (priName.compare(output->name()) == 0){
                priState = true;
            }
        }
        else{
            priState = output->isPrimary();
        }

        xrandrOutput::writeGlobalPart(output, info, nullptr);
        info[QStringLiteral("primary")] =  output->isPrimary();; //
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
        setOutputConfigInfo(output->isEnabled() ? output : nullptr);

//        if (output->isEnabled()) {
//            // try to update global output data
//            xrandrOutput::writeGlobal(output);
//        }

        outputList.append(info);
    }

    if (mAddScreen)
        mAddScreen = false;


    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
         file.write(QJsonDocument::fromVariant(outputList).toJson());
    } else {
         USD_LOG(LOG_DEBUG,"write file [%s] fail.cuz:%s.",file.fileName().toLatin1().data(),file.errorString().toLatin1().data());
    }


    if (screenConnectedCount > 1) {
        QFile backFile(fileModeConfigPath());
        if (backFile.open(QIODevice::WriteOnly)) {
            backFile.write(QJsonDocument::fromVariant(outputList).toJson());
        } else {
            USD_LOG(LOG_DEBUG,"write file [%s] fail.cuz:%s.",file.fileName().toLatin1().data(),backFile.errorString().toLatin1().data());
        }
    }

    USD_LOG(LOG_DEBUG,"write file:\n%s ok \n%s ok.",filePath.toLatin1().data(),mScreenMode == nullptr? "" : fileModeConfigPath().toLatin1().data());
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
