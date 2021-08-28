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

    QPoint getWinPos();
    QString getCurrentPrimaryScreenName();
    int getCurrentScale();

    void setupComponent();
    void setupConnect();

    int getCurrentStatus();

    void setCurrentUIStatus(int id);
    void setCurrentFirstOutputTip();

    void syncPrimaryScreenData(QString pName);

public:
    void setConfig(const KScreen::ConfigPtr &config);
    KScreen::ConfigPtr currentConfig() const;

    void setCloneModeSetup();
    void setExtendModeSetup();
    void setFirstModeSetup();
    void setOtherModeSetup();
    void setLeftExtendModeSetup();

private:
    QString findFirstOutput();
    QSize findBestCloneSize();
    bool turnonSpecifiedOutput(const KScreen::OutputPtr &output, int x, int y);

    int turnonAndGetRightmostOffset(const KScreen::OutputPtr &output, int x);

private:
    Ui::KDSWidget *ui;
    QButtonGroup * btnsGroup;

private:
    KScreen::ConfigPtr mConfig = nullptr;

public slots:
    void msgReceiveAnotherOne(const QString &msg);

private slots:
    void nextSelectedOption();
    void lastSelectedOption();
    void confirmCurrentOption();
    void receiveButtonClick(int x, int y);
    void closeApp();

Q_SIGNALS:
    void tellBtnClicked(int id);

};

#endif // KDSWIDGET_H
