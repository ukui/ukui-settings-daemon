#include "kdswidget.h"
//#include "ui_kdswidget.h"

#include <QScreen>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <kwindowsystem.h>

#include "clib-syslog.h"
#define TITLEHEIGHT 90
#define OPTIONSHEIGHT 70
#define BOTTOMHEIGHT 60


enum {
    FIRSTSCREEN,
    CLONESCREEN,
    EXTENDSCREEN,
//    LEXTENDSCREEN,
    OTHERSCREEN,
    ALLMODES,
};

bool operator<(const QSize &s1, const QSize &s2)
{
    return s1.width() * s1.height() < s2.width() * s2.height();
}

template<>
bool qMapLessThanKey(const QSize &s1, const QSize &s2)
{
    return s1 < s2;
}

KDSWidget::KDSWidget(QWidget *parent) :
    QWidget(parent)
{
//    ui->setupUi(this);

}

KDSWidget::~KDSWidget()
{
//    delete ui;
}

void KDSWidget::beginSetupKF5(){
    QObject::connect(new KScreen::GetConfigOperation(), &KScreen::GetConfigOperation::finished,
                     [&](KScreen::ConfigOperation *op) {
//        setConfig(qobject_cast<KScreen::GetConfigOperation*>(op)->config());

        if (mMonitoredConfig) {
            if (mMonitoredConfig->data()) {
                KScreen::ConfigMonitor::instance()->removeConfig(mMonitoredConfig->data());
                for (const KScreen::OutputPtr &output : mMonitoredConfig->data()->outputs()) {
                    output->disconnect(this);
                }
                mMonitoredConfig->data()->disconnect(this);
            }
            mMonitoredConfig = nullptr;
        }

        mMonitoredConfig = std::unique_ptr<xrandrConfig>(new xrandrConfig(qobject_cast<KScreen::GetConfigOperation*>(op)->config()));
        mMonitoredConfig->setValidityFlags(KScreen::Config::ValidityFlag::RequireAtLeastOneEnabledScreen);
        mMonitoredConfig->writeFile(false);
        exit(0);
    });
}


