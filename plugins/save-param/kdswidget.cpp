#include <QScreen>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <kwindowsystem.h>

#include "kdswidget.h"
#include "clib-syslog.h"

SaveScreenParam::SaveScreenParam(QObject *parent)
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
        m_MonitoredConfig->writeFile(false);
        exit(0);
    });
}


