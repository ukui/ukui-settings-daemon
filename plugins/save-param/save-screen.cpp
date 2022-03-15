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

#include "xrandrouput.h"
#include "save-screen.h"
#include "clib-syslog.h"



SaveScreenParam::SaveScreenParam(QObject *parent): QObject(parent)
{
    Q_UNUSED(parent);

    m_userName = "";

    m_isGet = false;
    m_isSet = false;
}

SaveScreenParam::~SaveScreenParam()
{
    XCloseDisplay(m_pDpy);
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

        if (false == m_userName.isEmpty()) {
            USD_LOG(LOG_DEBUG,".");
            m_MonitoredConfig->setUserName(m_userName);
            readConfigAndSet();
        }
        else if (isSet()) {
            SYS_LOG(LOG_DEBUG,".");
            readConfigAndSet();
        } else if (isGet()) {
            SYS_LOG(LOG_DEBUG,".");
            m_MonitoredConfig->writeFileForLightDM(false);
            exit(0);
        } else {
            m_MonitoredConfig->writeFile(false);
            exit(0);
        }
    });
}

void SaveScreenParam::setClone()
{
    initXparam();
    readConfigAndSet();
    exit(0);
}

void SaveScreenParam::setUserConfigParam()
{
    if (false ==initXparam()) {
        exit(0);
    }

    readConfigAndSet();
    exit(0);
}

void SaveScreenParam::setUserName(QString str)
{
    m_userName = str;
    USD_LOG_SHOW_PARAMS(m_userName.toLatin1().data());
}

void SaveScreenParam::setScreenSize()
{
    int screenInt = DefaultScreen (m_pDpy);
    if (m_kscreenConfigParam.m_screenWidth != DisplayWidth(m_pDpy, screenInt) ||
            m_kscreenConfigParam.m_screenHeight != DisplayHeight(m_pDpy, screenInt)) {
        int fb_width_mm;
        int fb_height_mm;

        double dpi = (25.4 * DisplayHeight(m_pDpy, screenInt)) / DisplayHeightMM(m_pDpy,screenInt);
        fb_width_mm = (25.4 * m_kscreenConfigParam.m_screenWidth) /dpi;
        fb_height_mm = (25.4 * m_kscreenConfigParam.m_screenHeight) /dpi;
        //dpi = Dot Per Inch，一英寸是2.54cm即25.4mm
        XRRSetScreenSize(m_pDpy, m_rootWindow, m_kscreenConfigParam.m_screenWidth, m_kscreenConfigParam.m_screenHeight,
                        fb_width_mm, fb_height_mm);

        SYS_LOG(LOG_DEBUG,"set screen size: %dx%d,%dx%d",m_kscreenConfigParam.m_screenWidth, m_kscreenConfigParam.m_screenHeight,
                fb_width_mm, fb_height_mm);
    }
}

void SaveScreenParam::readConfigAndSet()
{
    int ret;
    int tempInt;
    int tempCircle;
    bool hadFindFirstOutput = false;
    QFile file;
    QString configFullPath = "";

    if (m_KscreenConfigFile.isEmpty()) {
        return;
    }

    configFullPath = QString("/var/lib/lightdm-data/%1/usd/kscreen/%2").arg(m_userName).arg(m_KscreenConfigFile);
    file.setFileName(configFullPath);

    if (!file.open(QIODevice::ReadOnly)) {
        SYS_LOG(LOG_DEBUG,"can't open %s's screen config>%s",m_userName.toLatin1().data(),configFullPath.toLatin1().data());
        return;
    }

    SYS_LOG(LOG_DEBUG,"open %s's screen config>%s ok...",m_userName.toLatin1().data(),configFullPath.toLatin1().data());
    QJsonDocument parser;
    QVariantList outputsInfo = parser.fromJson(file.readAll()).toVariant().toList();

    SYS_LOG(LOG_DEBUG,"%dx%d",DisplayWidth(m_pDpy, DefaultScreen (m_pDpy)),DisplayHeight(m_pDpy, DefaultScreen (m_pDpy)));
    SYS_LOG(LOG_DEBUG,"%dmmx%dmm",DisplayWidthMM(m_pDpy, DefaultScreen (m_pDpy)),DisplayHeightMM(m_pDpy, DefaultScreen (m_pDpy)));

    for (const auto &variantInfo : outputsInfo) {
        UsdOuputProperty *outputProperty;
        outputProperty = new UsdOuputProperty();

        const QVariantMap info = variantInfo.toMap();
        const QVariantMap posInfo = info[QStringLiteral("pos")].toMap();
        const QVariantMap mateDataInfo = info[QStringLiteral("metadata")].toMap();
        const QVariantMap modeInfo = info[QStringLiteral("mode")].toMap();
        const QVariantMap sizeInfo = modeInfo[QStringLiteral("size")].toMap();

        outputProperty->setProperty("x", posInfo[QStringLiteral("x")].toString());
        outputProperty->setProperty("y", posInfo[QStringLiteral("y")].toString());
        outputProperty->setProperty("width", sizeInfo[QStringLiteral("width")].toString());
        outputProperty->setProperty("height", sizeInfo[QStringLiteral("height")].toString());
        outputProperty->setProperty("name", mateDataInfo[QStringLiteral("name")].toString());
        outputProperty->setProperty("primary", info[QStringLiteral("primary")].toString());
        outputProperty->setProperty("rotation", info[QStringLiteral("rotation")].toString());
        outputProperty->setProperty("rate", modeInfo[QStringLiteral("refresh")].toString());

        if (outputProperty->property("width").toInt() + outputProperty->property("x").toInt() > m_kscreenConfigParam.m_screenWidth) {
            m_kscreenConfigParam.m_screenWidth = outputProperty->property("width").toInt() + outputProperty->property("x").toInt();
        }

        if (outputProperty->property("height").toInt() + outputProperty->property("y").toInt() > m_kscreenConfigParam.m_screenHeight) {
            m_kscreenConfigParam.m_screenHeight = outputProperty->property("height").toInt() + outputProperty->property("y").toInt();
        }

        m_kscreenConfigParam.m_outputList.append(outputProperty);
//        USD_LOG_SHOW_PARAMS(outputProperty->property("name").toString().toLatin1().data());
//        USD_LOG_SHOW_PARAMS(outputProperty->property("x").toString().toLatin1().data());
//        USD_LOG_SHOW_PARAMS(outputProperty->property("y").toString().toLatin1().data());
//        USD_LOG_SHOW_PARAMS(outputProperty->property("width").toString().toLatin1().data());
//        USD_LOG_SHOW_PARAMS(outputProperty->property("height").toString().toLatin1().data());
//        USD_LOG_SHOW_PARAMS(outputProperty->property("primary").toString().toLatin1().data());
//        USD_LOG_SHOW_PARAMS(outputProperty->property("rotation").toString().toLatin1().data());
    }

    SYS_LOG(LOG_DEBUG,"%dx%d",DisplayWidth(m_pDpy, DefaultScreen (m_pDpy)),DisplayHeight(m_pDpy, DefaultScreen (m_pDpy)));
    SYS_LOG(LOG_DEBUG,"%dmmx%dmm",DisplayWidthMM(m_pDpy, DefaultScreen (m_pDpy)),DisplayHeightMM(m_pDpy, DefaultScreen (m_pDpy)));


//    XGrabServer(m_pDpy);
//所谓crtc ，就是一个显示规则：规定了某个显示器的显示模式，显示器名称，显示方位
    //停止所有输出，避免调整分辨率，刷新率，方向时产生花屏的情况
//    for (tempInt = 0; tempInt < m_pScreenRes->ncrtc; tempInt++) {
//        int ret = 0;
//        ret = XRRSetCrtcConfig (m_pDpy, m_pScreenRes, m_pScreenRes->crtcs[tempInt], CurrentTime,
//                         0, 0, None, RR_Rotate_0, NULL, 0);
//        if (ret != RRSetConfigSuccess) {
//            SYS_LOG(LOG_ERR,"disable crtc:%d error! ");
//        }
//    }

    setScreenSize();

    XRRSetOutputPrimary(m_pDpy, m_rootWindow, None);

    for (tempInt = 0; tempInt < m_pScreenRes->ncrtc; tempInt++) {
        RRMode monitorMode = 0;
        XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(m_pDpy, m_pScreenRes, m_pScreenRes->crtcs[tempInt]);


        if (crtcInfo->noutput == 0) {
            SYS_LOG(LOG_DEBUG, "skip %d,cuz %d:%d", tempInt,crtcInfo->noutput,m_kscreenConfigParam.m_outputList.count());
            continue;
        }

        for (int circle = 0; circle < crtcInfo->noutput; circle++) {
            UsdOuputProperty *kscreenOutputParam = nullptr;
            XRROutputInfo	*outputInfo = XRRGetOutputInfo (m_pDpy, m_pScreenRes, crtcInfo->outputs[circle]);
            QString  outputName = QString(QLatin1String(outputInfo->name));

            //遍历配置文件找到与X的对应的输出名
            for (int k = 0; k < m_kscreenConfigParam.m_outputList.count(); k++) {
                if (outputName != m_kscreenConfigParam.m_outputList[k]->property("name").toString()) {
                    continue;
                }

                kscreenOutputParam = m_kscreenConfigParam.m_outputList[k];
                break;
            }

           monitorMode = getModeId(outputInfo, kscreenOutputParam);
           if (0 == monitorMode) {
                SYS_LOG(LOG_ERR,"mode id get error..");
                return;
           }

           /*10个参数：
            Display：display
            XRRScreenResources *resources：屏幕资源
            RRCrtc crtc：crtc
            Time timestamp：
            int x, int y：坐标
            RRMode mode：模式
            Rotation rotation：角度
            RROutput *outputs：输出设备的ID地址
            int noutputs：crtc中的outputs数量
           */
           ret = XRRSetCrtcConfig (m_pDpy, m_pScreenRes, m_pScreenRes->crtcs[tempInt], CurrentTime,
                                   kscreenOutputParam->property("x").toInt(), kscreenOutputParam->property("y").toInt(), monitorMode, RR_Rotate_0, crtcInfo->outputs, crtcInfo->noutput);

           if (kscreenOutputParam->property("primary").toString() == "true") {
               XRRSetOutputPrimary(m_pDpy, m_rootWindow, crtcInfo->outputs[0]);

           }

           if (ret != RRSetConfigSuccess) {
                SYS_LOG(LOG_ERR,"%s RRSetConfigFail..",kscreenOutputParam->property("name").toString().toLatin1().data());
           } else {
               SYS_LOG(LOG_ERR,"%s RRSetConfigSuccess",kscreenOutputParam->property("name").toString().toLatin1().data());
           }
        }
    }

return;

#if 0
    //screen size 不需要设置，最大尺寸不受影响。
    //显示模式都挂在屏幕资源内，并非显示器所支持的list内。
    for (tempInt = 0; tempInt < m_pScreenRes->noutput; tempInt++) {
        int ret = 0;
        RRMode monitorMode = NULL;
        QString outputName;
        UsdOuputProperty *kscreenOutputParam = nullptr;
        XRROutputInfo	*outputInfo = XRRGetOutputInfo (m_pDpy, m_pScreenRes, m_pScreenRes->outputs[tempInt]);
        outputName = QString(QLatin1String(outputInfo->name));

        //遍历配置文件找到与X的对应的输出名
        for (int k = 0; k < m_kscreenConfigParam.m_outputList.count(); k++) {
            if (outputName != m_kscreenConfigParam.m_outputList[k]->property("name").toString()) {
                continue;
            }

            kscreenOutputParam = m_kscreenConfigParam.m_outputList[k];
            break;
        }

        if (kscreenOutputParam == nullptr) {
            SYS_LOG(LOG_DEBUG,"can't find %s param in config", outputInfo->name);
            continue;
        } else {
             SYS_LOG(LOG_DEBUG,"find %s param in config", outputInfo->name);
        }


        double rate;
        double vTotal;
        XRRModeInfo *xmodeInfo;


        for (int m = 0; m < m_pScreenRes->nmode; m++) {
            XRRModeInfo *mode = &m_pScreenRes->modes[m];
            vTotal = mode->vTotal;

            if (mode->modeFlags & RR_DoubleScan) {
                /* doublescan doubles the number of lines */
                vTotal *= 2;
            }

            if (mode->modeFlags & RR_Interlace) {
                /* interlace splits the frame into two fields */
                /* the field rate is what is typically reported by monitors */
                vTotal /= 2;
            }

            rate = mode->dotClock / ((double) mode->hTotal * (double) vTotal);
            if (mode->width == kscreenOutputParam->property("width").toInt()
                    && mode->height == kscreenOutputParam->property("height").toInt()) {
                double kscreenRate = kscreenOutputParam->property("rate").toDouble();
                if (kscreenRate == rate) {
                    monitorMode = mode->id;
                    SYS_LOG(LOG_DEBUG,"find same rate %f",kscreenRate);
                    break;
                } else {
                    SYS_LOG(LOG_DEBUG,"%dx%d %f!=%f",mode->width,mode->height,rate,kscreenRate);

                }
            }
        }



        USD_LOG_SHOW_PARAM1(m_pScreenRes->crtcs[tempInt]);
        USD_LOG_SHOW_PARAM1(monitorMode);
        USD_LOG(LOG_DEBUG,"outputs:%d,%d,%d,%d,", *(m_pScreenRes->outputs),*(m_pScreenRes->outputs+1),*(m_pScreenRes->outputs+2),*(m_pScreenRes->outputs+3));
        USD_LOG(LOG_DEBUG,"crtcs:%d,%d,%d,%d,", *(m_pScreenRes->crtcs),*(m_pScreenRes->crtcs+1),*(m_pScreenRes->crtcs+2),*(m_pScreenRes->crtcs+3));


        for (tempCircle = 0; tempCircle < m_pScreenRes->ncrtc; tempCircle++) {
             XRRCrtcInfo *crtcInfo = XRRGetCrtcInfo(m_pDpy, m_pScreenRes, m_pScreenRes->crtcs[tempCircle]);
//             crtcInfo->outputs
        }

        ret = XRRSetCrtcConfig (m_pDpy, m_pScreenRes, m_pScreenRes->crtcs[0], CurrentTime,
                                0, 0, monitorMode, RR_Rotate_0, m_pScreenRes->outputs+tempInt, 1);

        if (ret != RRSetConfigSuccess) {
            SYS_LOG(LOG_ERR,"disable crtc:%d error! ",ret);
        } else {
            SYS_LOG(LOG_ERR,"RRSetConfigSuccess");
        }
    }

    /*
     *
        XGrabServer (dpy);
        for (c = 0; c < res->ncrtc; c++) {
            crtc_t	    *crtc = &crtcs[c];
            s = crtc_disable (crtc);//XRRSetCrtcConfig (dpy, res, crtc->crtc.xid, CurrentTime, 0, 0, None, RR_Rotate_0, NULL, 0);
            if (s != RRSetConfigSuccess)
                panic (s, crtc);
            }
        }

        XRRSetScreenSize (dpy, root, fb_width, fb_height,
              fb_width_mm, fb_height_mm);

        screen_apply();
        for (c = 0; c < res->ncrtc; c++)
            {
            crtc_t	*crtc = &crtcs[c];

            s = crtc_apply (crtc);
            if (s != RRSetConfigSuccess)
                panic (s, crtc);
            }
      XRRSetOutputPrimary(dpy, root, output->output.xid);
      XRRSetScreenSize (dpy, root, fb_width, fb_height,
                  fb_width_mm, fb_height_mm);
       XUngrabServer (dpy);
     */
#endif
}

void SaveScreenParam::readConfigAndSetBak()
{
    if (m_MonitoredConfig->lightdmFileExists()) {
        std::unique_ptr<xrandrConfig> monitoredConfig = m_MonitoredConfig->readFile(false);

        if (monitoredConfig == nullptr ) {
            USD_LOG(LOG_DEBUG,"config a error");
            exit(0);
            return;
        }

        m_MonitoredConfig = std::move(monitoredConfig);
        if (m_MonitoredConfig->canBeApplied()) {
            connect(new KScreen::SetConfigOperation(m_MonitoredConfig->data()),
                    &KScreen::SetConfigOperation::finished,
                    this, [this]() {
                USD_LOG(LOG_DEBUG,"set success。。");
                exit(0);
            });
        } else {
            USD_LOG(LOG_ERR,"--|can't be apply|--");
            exit(0);
        }
    } else {
         exit(0);
    }
}

void SaveScreenParam::getRootWindows()
{

}

void SaveScreenParam::getScreen()
{
//    m_res = XRRGetScreenResourcesCurrent (m_dpy, root);
}

bool SaveScreenParam::initXparam()
{
    int             actualFormat;
    int             tempInt;
    uchar           *prop;

    Atom            tempAtom;
    Atom            actualType;

    ulong           nitems;
    ulong           bytesAfter;
    QStringList     outputsHashList;

    m_pDpy = XOpenDisplay (m_pDisplayName);
    if (m_pDpy == NULL) {
        SYS_LOG(LOG_DEBUG,"XOpenDisplay fail...");
        return false;
    }

    tempAtom = XInternAtom (m_pDpy, "EDID", false);

    m_screen = DefaultScreen(m_pDpy);
    if (m_screen >= ScreenCount (m_pDpy)) {
        SYS_LOG(LOG_DEBUG,"Invalid screen number %d (display has %d)",m_screen, ScreenCount(m_pDpy));
        return false;
    }

    m_rootWindow = RootWindow(m_pDpy, m_screen);

    m_pScreenRes = XRRGetScreenResources(m_pDpy, m_rootWindow);
    if (NULL == m_pScreenRes) {
        SYS_LOG(LOG_DEBUG,"could not get screen resources",m_screen, ScreenCount(m_pDpy));
        return false;
    }

    if (m_pScreenRes->noutput == 0) {
        SYS_LOG(LOG_DEBUG,"noutput is 0!!");
        return false;
    }

    for (tempInt = 0; tempInt < m_pScreenRes->noutput; tempInt++) {
       XRRGetOutputProperty(m_pDpy, m_pScreenRes->outputs[tempInt], tempAtom,
                 0, 100, False, False,
                 AnyPropertyType,
                 &actualType, &actualFormat,
                 &nitems, &bytesAfter, &prop);

       if (nitems == 0) {
           continue;
       }

       QCryptographicHash hash(QCryptographicHash::Md5);
       hash.addData(reinterpret_cast<const char *>(prop), nitems);
       QString checksum = QString::fromLatin1(hash.result().toHex());
       outputsHashList.append(checksum);
    }

    if (outputsHashList.count() == 0) {
        SYS_LOG(LOG_DEBUG,"outputsHashList is empty");
        return false;
    }

    std::sort(outputsHashList.begin(), outputsHashList.end());
    const auto configHash = QCryptographicHash::hash(outputsHashList.join(QString()).toLatin1(),
                                               QCryptographicHash::Md5);

    m_KscreenConfigFile = QString::fromLatin1(configHash.toHex());
    SYS_LOG(LOG_DEBUG,"initXparam success");
    return true;
}

int SaveScreenParam::crtcDisable()
{

}

int SaveScreenParam::crtcApply()
{

}

//所有显示器的模式都放在一起，需要甄别出需要的显示器，避免A取到B的模式。需要判断那个显示器支持那些模式
RRMode SaveScreenParam::getModeId(XRROutputInfo	*outputInfo,UsdOuputProperty *kscreenOutputParam)
{
    double rate;
    double vTotal;

    RRMode ret = 0;

    for (int m = 0; m < m_pScreenRes->nmode; m++) {
        XRRModeInfo *mode = &m_pScreenRes->modes[m];
        vTotal = mode->vTotal;
//        SYS_LOG(LOG_DEBUG,"start check mode:%s id:%d for %s", mode->name, mode->id, outputInfo->name);
        if (mode->modeFlags & RR_DoubleScan) {
            /* doublescan doubles the number of lines */
            vTotal *= 2;
        }

        if (mode->modeFlags & RR_Interlace) {
            /* interlace splits the frame into two fields */
            /* the field rate is what is typically reported by monitors */
            vTotal /= 2;
        }

        rate = mode->dotClock / ((double) mode->hTotal * (double) vTotal);
        if (mode->width == kscreenOutputParam->property("width").toInt()
                && mode->height == kscreenOutputParam->property("height").toInt()) {
            double kscreenRate = kscreenOutputParam->property("rate").toDouble();

            if (qAbs(kscreenRate - rate) < 0.2) {
                for (int k = 0; k< outputInfo->nmode; k++) {
                    if (outputInfo->modes[k] == mode->id) {
                        SYS_LOG(LOG_DEBUG,"find %s mode:%s id:%d",outputInfo->name, mode->name, mode->id);
                        return mode->id;
                    }
                }
            } else {
                SYS_LOG(LOG_DEBUG,"%dx%d %f!=%f",mode->width,mode->height,rate,kscreenRate);
            }
        }
    }

    return ret;
}
//crct和output是分开的，但是设置时是需要同时设置进去
QString SaveScreenParam::getConfigFileName()
{
    int             actualFormat;
    int             tempInt;
    int             tempCircle;
    uchar           *prop;

    Atom            tempAtom;
    Atom            actualType;

    ulong           nitems;
    ulong           bytesAfter;
    QStringList     outputsHashList;
    m_pDpy = XOpenDisplay (m_pDisplayName);

    if (m_pDpy == NULL) {
        SYS_LOG(LOG_ERR,"XOpenDisplay fail...");
        return "";
    }
    tempAtom = XInternAtom (m_pDpy, "EDID", false);

    m_screen = DefaultScreen(m_pDpy);
    if (m_screen >= ScreenCount (m_pDpy)) {
        XCloseDisplay(m_pDpy);
        SYS_LOG(LOG_ERR,"Invalid screen number %d (display has %d)",m_screen, ScreenCount(m_pDpy));
        return "";
    }

    m_rootWindow = RootWindow(m_pDpy, m_screen);

    m_pScreenRes = XRRGetScreenResourcesCurrent (m_pDpy, m_rootWindow);
    if (NULL == m_pScreenRes) {
        XCloseDisplay(m_pDpy);
        SYS_LOG(LOG_ERR,"could not get screen resources",m_screen, ScreenCount(m_pDpy));
        return "";
    }

    for (tempInt = 0; tempInt < m_pScreenRes->noutput; tempInt++) {
//       XRROutputInfo	*outputInfo = XRRGetOutputInfo(m_pDpy, m_pScreenRes, m_pScreenRes->outputs[tempInt]);

       XRRGetOutputProperty(m_pDpy, m_pScreenRes->outputs[tempInt], tempAtom,
                 0, 100, False, False,
                 AnyPropertyType,
                 &actualType, &actualFormat,
                 &nitems, &bytesAfter, &prop);

       if (nitems == 0) {
           continue;
       }
/*
       for (tempCircle = 0; tempCircle < nitems; tempCircle++) {
            if (tempCircle != 0 && tempCircle %16 ==0) {
                printf("\r\n");
            }
            printf("%02x",prop[tempCircle]);
       }
*/
       QCryptographicHash hash(QCryptographicHash::Md5);
       hash.addData(reinterpret_cast<const char *>(prop), nitems);
       QString checksum = QString::fromLatin1(hash.result().toHex());
       USD_LOG_SHOW_PARAMS(checksum.toLatin1().data());
       outputsHashList.append(checksum);
    }


    if (outputsHashList.count() == 0) {
        return "";
    }

    std::sort(outputsHashList.begin(), outputsHashList.end());
    const auto configHash = QCryptographicHash::hash(outputsHashList.join(QString()).toLatin1(),
                                               QCryptographicHash::Md5);

    QString configHashQStr = QString::fromLatin1(configHash.toHex());
    USD_LOG_SHOW_PARAMS(configHashQStr.toLatin1().data());

    return configHashQStr;
}

QString SaveScreenParam::getScreenParamFromConfigName(QString configFileName)
{

}



