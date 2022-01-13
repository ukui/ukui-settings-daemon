#ifndef KDSWIDGET_H
#define KDSWIDGET_H

#include <KF5/KScreen/kscreen/output.h>
#include <KF5/KScreen/kscreen/edid.h>
#include <KF5/KScreen/kscreen/mode.h>
#include <KF5/KScreen/kscreen/config.h>
#include <KF5/KScreen/kscreen/getconfigoperation.h>
#include <KF5/KScreen/kscreen/setconfigoperation.h>
#include "xrandr-config.h"


class SaveScreenParam : QObject
{
    Q_OBJECT

public:
    explicit SaveScreenParam(QObject *parent = nullptr);
    ~SaveScreenParam();

    void getConfig();
private:
    std::unique_ptr<xrandrConfig> m_MonitoredConfig = nullptr;
};

#endif // KDSWIDGET_H
