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

#include <QScreen>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <kwindowsystem.h>
#include <QFile>
#include <QDir>
#include <QtXml>

#include "save-screen.h"
#include "clib-syslog.h"



SaveScreenParam::SaveScreenParam(QObject *parent): QObject(parent)
{
    Q_UNUSED(parent);
}

SaveScreenParam::~SaveScreenParam()
{

}



void SaveScreenParam::getConfig(){
    QObject::connect(new KScreen::GetConfigOperation(), &KScreen::GetConfigOperation::finished,
                     [&](KScreen::ConfigOperation *op) {
        if (m_MonitoredConfig) {
            if (m_MonitoredConfig->data()) {
                KScreen::ConfigMonitor::instance()->removeConfig(m_MonitoredConfig->data());
                for (const KScreen::OutputPtr &output : m_MonitoredConfig->data()->outputs()) {
                    output->disconnect(this);
                }
                m_MonitoredConfig->data()->disconnect(this);
            }
            m_MonitoredConfig = nullptr;
        }
        m_MonitoredConfig = std::unique_ptr<xrandrConfig>(new xrandrConfig(qobject_cast<KScreen::GetConfigOperation*>(op)->config()));
        m_MonitoredConfig->setValidityFlags(KScreen::Config::ValidityFlag::RequireAtLeastOneEnabledScreen);
        if (isSet()) {
            USD_LOG_SHOW_PARAM1(m_isSet);
            readConfigAndSet();
        } else if (isGet()) {
            USD_LOG_SHOW_PARAM1(m_isGet);
            m_MonitoredConfig->writeFileForLightDM(false);
            exit(0);
        } else {
            m_MonitoredConfig->writeFile(false);
            exit(0);
        }
    });
}

void SaveScreenParam::readConfigAndSet()
{
    if (m_MonitoredConfig->lightdmFileExists()) {
//        USD_LOG(LOG_DEBUG,"read  config:%s.",m_MonitoredConfig->filePath().toLatin1().data());

        std::unique_ptr<xrandrConfig> MonitoredConfig = m_MonitoredConfig->readFile(false);

        if (MonitoredConfig == nullptr ) {
            USD_LOG(LOG_DEBUG,"config a error");
            return;
        }

        m_MonitoredConfig = std::move(MonitoredConfig);


        if (m_MonitoredConfig->canBeApplied()) {
            connect(new KScreen::SetConfigOperation(m_MonitoredConfig->data()),
                    &KScreen::SetConfigOperation::finished,
                    this, [this]() {
                USD_LOG(LOG_DEBUG,"set success。。");
                exit(0);
            });
        } else {
            USD_LOG(LOG_ERR,"--|can't be apply|--");

            Q_FOREACH (const KScreen::OutputPtr &output, m_MonitoredConfig->data()->outputs()) {
                USD_LOG_SHOW_OUTPUT(output);
            }
        }
    }
}



