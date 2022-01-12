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
#include "xrandr-config.h"


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
private:
    std::unique_ptr<xrandrConfig> mMonitoredConfig = nullptr;
};

#endif // KDSWIDGET_H
