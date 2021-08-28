#include "kdswidget.h"
#include "ui_kdswidget.h"

#include <QScreen>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <kwindowsystem.h>

#include "expendbutton.h"

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
    QWidget(parent),
    ui(new Ui::KDSWidget)
{
    ui->setupUi(this);

}

KDSWidget::~KDSWidget()
{
    delete ui;
}

void KDSWidget::beginSetupKF5(){
    QObject::connect(new KScreen::GetConfigOperation(), &KScreen::GetConfigOperation::finished,
                     [&](KScreen::ConfigOperation *op) {
        setConfig(qobject_cast<KScreen::GetConfigOperation*>(op)->config());
    });

    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    setAttribute(Qt::WA_TranslucentBackground, true);

    /* 不在任务栏显示图标 */
    KWindowSystem::setState(winId(), NET::SkipTaskbar | NET::SkipPager);

    btnsGroup = new QButtonGroup;

    QDBusConnection::systemBus().connect(QString(), \
                                         QString(), \
                                         "org.ukui.kds.interface", \
                                         "signalNextOption", \
                                         this, SLOT(nextSelectedOption()));

    QDBusConnection::systemBus().connect(QString(), \
                                         QString(), \
                                         "org.ukui.kds.interface", \
                                         "signalLastOption", \
                                         this, SLOT(lastSelectedOption()));

    QDBusConnection::systemBus().connect(QString(), \
                                         QString(), \
                                         "org.ukui.kds.interface", \
                                         "signalCloseApp", \
                                         this, SLOT(closeApp()));

    QDBusConnection::systemBus().connect(QString(), \
                                         QString(), \
                                         "org.ukui.kds.interface", \
                                         "signalMakeClicked", \
                                         this, SLOT(confirmCurrentOption()));


    QDBusConnection::systemBus().connect(QString(), \
                                         QString(), \
                                         "org.ukui.kds.interface", \
                                         "signalButtonClicked", \
                                         this, SLOT(receiveButtonClick(int,int)));
}


QString KDSWidget::getCurrentPrimaryScreenName(){
    QDBusInterface usdiface("org.ukui.SettingsDaemon",
                            "/org/ukui/SettingsDaemon/wayland",
                            "org.ukui.SettingsDaemon.wayland",
                            QDBusConnection::sessionBus());

    if (usdiface.isValid()){
        QDBusReply<QString> reply = usdiface.call("priScreenName");
        if (reply.isValid()){
            return reply.value();
        }
    }

    return QString("");
}

int KDSWidget::getCurrentScale(){
    QDBusInterface usdiface("org.ukui.SettingsDaemon",
                            "/org/ukui/SettingsDaemon/wayland",
                            "org.ukui.SettingsDaemon.wayland",
                            QDBusConnection::sessionBus());

    if (usdiface.isValid()){
        QDBusReply<int> reply = usdiface.call("scale");
        if (reply.isValid()){
            return reply.value();
        }
    }

    return 1;
}

QPoint KDSWidget::getWinPos(){
    QString pName = getCurrentPrimaryScreenName();
    if (!pName.isEmpty()){

        const KScreen::ConfigPtr &config = this->currentConfig();

        Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
            if (QString::compare(output.data()->name(), pName) == 0){
                QRect rect = output.data()->geometry();
                return rect.center();
            }
        }
    }

    QScreen * pScreen = QGuiApplication::screens().at(0);
    return pScreen->geometry().center();
}

void KDSWidget::setupComponent(){

    ui->outputPrimaryTip->hide();
//    setCurrentFirstOutputTip();

    for (int i = 0; i < ALLMODES; i++){
        ExpendButton * btn = new ExpendButton();
        btn->setFixedHeight(70);
        btnsGroup->addButton(btn, i);

        switch (i) {
        case FIRSTSCREEN:
            btn->setSign(FIRSTSCREEN % 2);
            btn->setBtnText(tr("First Screen"));
            btn->setBtnLogo(":/img/main.png");
            break;
        case CLONESCREEN:
            btn->setSign(CLONESCREEN % 2);
            btn->setBtnText(tr("Clone Screen"));
            btn->setBtnLogo(":/img/clone.png");
            break;
        case EXTENDSCREEN:
            btn->setSign(EXTENDSCREEN % 2);
            btn->setBtnText(tr("Extend Screen"));
            btn->setBtnLogo(":/img/extend.png");
            break;
//        case LEXTENDSCREEN:
//            btn->setSign(LEXTENDSCREEN % 2);
//            btn->setBtnText(tr("Left Extend Screen"));
//            btn->setBtnLogo(":/img/extend.png");
//            break;
        case OTHERSCREEN:
            btn->setSign(OTHERSCREEN % 2);
            btn->setBtnText(tr("Vice Screen"));
            btn->setBtnLogo(":/img/vice.png");
            break;
        default:
            break;
        }

        ui->btnsVerLayout->addWidget(btn);
    }

    int h = TITLEHEIGHT + OPTIONSHEIGHT * ALLMODES + BOTTOMHEIGHT;
    setFixedWidth(400);
    setFixedHeight(h);

    /// QSS
    ui->titleFrame->setStyleSheet("QFrame#titleFrame{background: #A6000000; border: none; border-top-left-radius: 24px; border-top-right-radius: 24px;}");
    ui->bottomFrame->setStyleSheet("QFrame#bottomFrame{background: #A6000000; border: none; border-bottom-left-radius: 24px; border-bottom-right-radius: 24px;}");

    ui->splitFrame->setStyleSheet("QFrame#splitFrame{background: #99000000; border: none;}");

    ui->titleLabel->setStyleSheet("QLabel{color: #FFFFFF; font-size: 24px;}");
    ui->outputPrimaryTip->setStyleSheet("QLabel{color: #60FFFFFF; font-size: 18px;}");
    ui->outputName->setStyleSheet("QLabel{color: #60FFFFFF; font-size: 18px;}");
    ui->outputDisplayName->setStyleSheet("QLabel{color: #60FFFFFF; font-size: 18px;}");
}

void KDSWidget::setupConnect(){
    connect(btnsGroup, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), this, [=](int id){


        /* 获取旧选项 */
        for (QAbstractButton * button : btnsGroup->buttons()){
            ExpendButton * btn = dynamic_cast<ExpendButton *>(button);
//            qDebug() << "old index: " << btn->getBtnChecked();
            int index = btnsGroup->id(button);
            if (index == id && btn->getBtnChecked()){
                    goto closeapp;
            }
        }

        switch (id) {
        case FIRSTSCREEN:
            setFirstModeSetup();
            setCurrentUIStatus(FIRSTSCREEN);
            break;
        case CLONESCREEN:
            setCloneModeSetup();
            setCurrentUIStatus(CLONESCREEN);
            break;
        case EXTENDSCREEN:
            setExtendModeSetup();
            setCurrentUIStatus(EXTENDSCREEN);
            break;
//        case LEXTENDSCREEN:
//            setLeftExtendModeSetup();
//            setCurrentUIStatus(LEXTENDSCREEN);
//            break;
        case OTHERSCREEN:
            setOtherModeSetup();
            setCurrentUIStatus(OTHERSCREEN);
            break;
        default:
            break;
        }


closeapp:
        close();

    });
}

int KDSWidget::getCurrentStatus(){

    QString firstOutputName;

    const KScreen::ConfigPtr &config = this->currentConfig();

    const KScreen::OutputList &outputs = config->connectedOutputs();

    firstOutputName = findFirstOutput();

    if (outputs.count() < 2){
        return FIRSTSCREEN;
    } else {
        /* output.data()->clones().isEmpty() is not valid */
        Q_FOREACH(const KScreen::OutputPtr &output, outputs) {

//            if (!output.data()->clones().isEmpty()){
//                return CLONESCREEN;
//            }

            if (QString::compare(firstOutputName, output.data()->name()) == 0){
                if (!output.data()->isEnabled()){
                    return OTHERSCREEN;
                }
            }

            if (!output.data()->isEnabled()){
                return FIRSTSCREEN;
            }
        }

        Q_FOREACH(const KScreen::OutputPtr &output, outputs) {
            if (output.data()->pos().x() != 0){
                return EXTENDSCREEN;
            }
        }

        return CLONESCREEN;
    }

//    Q_FOREACH(const KScreen::OutputPtr &output, outputs) {

//        if (QString::compare(firstOutputName, output.data()->name()) == 0){
//            QPoint pPos = output.data()->pos();
//            if (pPos.x() > 0){
//                return LEXTENDSCREEN;
//            } else {
//                return REXTENDSCREEN;
//            }
//        }
//    }


}

void KDSWidget::setCurrentUIStatus(int id){
    //set all no checked
    for (QAbstractButton * button : btnsGroup->buttons()){
        ExpendButton * btn = dynamic_cast<ExpendButton *>(button);

        btn->setBtnChecked(false);

        if (id == btnsGroup->id(button)){
            btn->setBtnChecked(true);
            btn->setChecked(true);
        }
    }

    // status == -1
    if (id == -1){
        ExpendButton * btn1 = dynamic_cast<ExpendButton *>(btnsGroup->button(FIRSTSCREEN));
//        btn1->setBtnChecked(true);
        btn1->setChecked(true);
    }
}

void KDSWidget::setConfig(const KScreen::ConfigPtr &config) {

    mConfig = config;


    //获取主屏位置
    QPoint point = getWinPos();
    move(point.x() - width()/2, point.y() - height()/2);

    setupComponent();
    setupConnect();

    setCurrentUIStatus(getCurrentStatus());
}

KScreen::ConfigPtr KDSWidget::currentConfig() const {

    return mConfig;
}

void KDSWidget::setCurrentFirstOutputTip(){

    const KScreen::ConfigPtr &config = this->currentConfig();

    QString firstOutputName = findFirstOutput();

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        QString opName = output.data()->name();
        QString opDisName = output.data()->edid()->name();

        if (QString::compare(firstOutputName, output.data()->name()) == 0){
            ui->outputName->setText(opName);
            ui->outputDisplayName->setText(""/*opDisName*/);

            return;
        }
    }

    ui->outputName->setText(tr("N/A"));

}

QString KDSWidget::findFirstOutput(){

    int firstopID = -1;
    QString firstopName;

    const KScreen::ConfigPtr &config = this->currentConfig();

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        if (firstopID == -1){
            firstopID = output.data()->id();
            firstopName = output.data()->name();
        }
        if (firstopID > output.data()->id()){
            firstopID = output.data()->id();
            firstopName = output.data()->name();
        }
    }

    return firstopName;
}

void KDSWidget::setCloneModeSetup(){

    QList<int> clones;

    const KScreen::ConfigPtr &config = this->currentConfig();

    QSize cloneSize = findBestCloneSize();

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {

        float bestRate = 0.0;
        QString bestID;

        output.data()->setEnabled(true);

        Q_FOREACH (const KScreen::ModePtr &mode, output->modes()) {

            if (mode.data()->size() == cloneSize){

                float r = mode.data()->refreshRate();
                if (bestRate < r){
                    bestRate = r;
                    bestID = mode.data()->id();
                }

            }
        }

        if (bestRate > 0){

            output.data()->setCurrentModeId(bestID);
            output.data()->setRotation(KScreen::Output::None);
            output.data()->setPos(QPoint(0, 0));

            if (!output.data()->isPrimary()){
                clones.append(output.data()->id());
            }
        }

    }

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {

        if (output.data()->isPrimary()){
            output.data()->setClones(clones);
        } else {
            output.data()->setClones(QList<int>());
        }

    }



    if (!KScreen::Config::canBeApplied(config)) {
//        qDebug() << "Can not apply!";
        return;
    }

    auto *op = new KScreen::SetConfigOperation(config);

    op->exec();

    syncPrimaryScreenData(getCurrentPrimaryScreenName());

}

void KDSWidget::setExtendModeSetup(){
    const KScreen::ConfigPtr &config = this->currentConfig();

    int x = 0;

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        if (!output.data()->isPrimary()){
            continue;
        }

        output.data()->setEnabled(true);
        output.data()->setClones(QList<int>());

        x = turnonAndGetRightmostOffset(output, x);

    }

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        if (output.data()->isPrimary()){
            continue;
        }

        output.data()->setEnabled(true);
        output.data()->setClones(QList<int>());

        x = turnonAndGetRightmostOffset(output, x);

    }

    if (!KScreen::Config::canBeApplied(config)) {
//        qDebug() << "Can not apply!";
        return;
    }

    auto *op = new KScreen::SetConfigOperation(config);

    op->exec();

    //
    syncPrimaryScreenData(getCurrentPrimaryScreenName());
}

void KDSWidget::setLeftExtendModeSetup(){
    const KScreen::ConfigPtr &config = this->currentConfig();

    int x = 0;


    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        if (output.data()->isPrimary()){
            continue;
        }

        output.data()->setEnabled(true);
        output.data()->setClones(QList<int>());

        x = turnonAndGetRightmostOffset(output, x);

    }

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        if (!output.data()->isPrimary()){
            continue;
        }

        output.data()->setEnabled(true);
        output.data()->setClones(QList<int>());

        x = turnonAndGetRightmostOffset(output, x);

    }

    if (!KScreen::Config::canBeApplied(config)) {
//        qDebug() << "Can not apply!";
        return;
    }

    auto *op = new KScreen::SetConfigOperation(config);

    op->exec();


    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        qDebug() << "\n" << output.data()->name() << output.data()->clones() << output.data()->clone();
    }
}

void KDSWidget::setFirstModeSetup(){
    const KScreen::ConfigPtr &config = this->currentConfig();

    QString firstName = findFirstOutput();

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        /* 取消镜像模式标志位 */
        output.data()->setClones(QList<int>());

        if (QString::compare(output.data()->name(), firstName) == 0){
            turnonSpecifiedOutput(output, 0, 0);
        } else {
            output.data()->setEnabled(false);
        }

    }

    if (!KScreen::Config::canBeApplied(config)) {
//        qDebug() << "Can not apply!";
        return;
    }

    auto * op = new KScreen::SetConfigOperation(config);
    op->exec();

    syncPrimaryScreenData(firstName);
}

void KDSWidget::setOtherModeSetup(){
    const KScreen::ConfigPtr &config = this->currentConfig();

    QString firstName = findFirstOutput();
    QString otherName;

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {

        /* 取消镜像模式标志位 */
        output.data()->setClones(QList<int>());

        if (QString::compare(output.data()->name(), firstName) == 0){
            output.data()->setEnabled(false);
        } else {
//            output.data()->setEnabled(true);
            turnonSpecifiedOutput(output, 0, 0);
        }

        //获取非主屏的Name。TODO:多屏(>2)情况下呢？
        if (QString::compare(output.data()->name(), firstName) == 0){

        } else {
            otherName = output.data()->name();
        }

    }

    if (!KScreen::Config::canBeApplied(config)) {
//        qDebug() << "Can not apply!";
        return;
    }

    auto * op = new KScreen::SetConfigOperation(config);

    op->exec();

    syncPrimaryScreenData(otherName);
}

int KDSWidget::turnonAndGetRightmostOffset(const KScreen::OutputPtr &output, int x){

    turnonSpecifiedOutput(output, x, 0);

    int width;
//    width = output.data()->size().width();
    width = output.data()->preferredMode().data()->size().width();

//    qDebug() << output.data()->name() << "width is " << output.data()->size() << output.data()->preferredMode().data()->size();
//    Q_FOREACH (const KScreen::ModePtr &mode, output->modes()) {
//        qDebug() << "mode is: " << mode.data()->id() << mode.data()->size();
//    }

    x += width;

    return x;
}

bool KDSWidget::turnonSpecifiedOutput(const KScreen::OutputPtr &output, int x, int y){

    output->setEnabled(true);
    output->setCurrentModeId(output.data()->preferredModeId());
    output->setRotation(KScreen::Output::None);
    output->setPos(QPoint(x, y));

    return true;

}

QSize KDSWidget::findBestCloneSize(){

    const KScreen::ConfigPtr &config = this->currentConfig();

    QMap<QSize, int> commonSizes;

    Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
        QList<QSize> pSizes;
        Q_FOREACH (const KScreen::ModePtr &mode, output->modes()) {

            QSize s = mode.data()->size();
            if (pSizes.contains(s))
                continue;

            pSizes.append(s);

            if (commonSizes.contains(s)){
                commonSizes[s]++;
            } else {
                commonSizes.insert(s, 1);
            }

        }
    }

    QList<QSize> commonResults = commonSizes.keys(config->connectedOutputs().count());

    QSize smallestSize;

    /* TODO: */
    if (commonSizes.isEmpty()){
//        Q_FOREACH (const KScreen::OutputPtr &output, config->connectedOutputs()) {
//            if (!smallestSize.isValid() || output->preferredMode()->size() < smallestSize) {
//                smallestSize = output->preferredMode()->size();
//            }
//        }
    } else {

        Q_FOREACH(QSize s1, commonResults){
            if (!smallestSize.isValid() || smallestSize < s1){
                smallestSize = s1;
            }
        }

    }

    return smallestSize;

}

void KDSWidget::nextSelectedOption(){
    int current = btnsGroup->checkedId();
    int next;

    /* no button checked */
//    if (current == -1)
//        ;

    next = current == ALLMODES - 1 ? 0 : current + 1;

    ExpendButton * btn = dynamic_cast<ExpendButton *>(btnsGroup->button(next));
    btn->setChecked(true);
}

void KDSWidget::lastSelectedOption(){
    int current = btnsGroup->checkedId();
    int last;

    /* no button checked */
    if (current == -1)
        current = ALLMODES;

    last = current == 0 ? ALLMODES - 1 : current - 1;

    ExpendButton * btn = dynamic_cast<ExpendButton *>(btnsGroup->button(last));
    btn->setChecked(true);
}

void KDSWidget::confirmCurrentOption(){
    int current = btnsGroup->checkedId();
//    qDebug() << "current checked" << current;

    if (current == -1)
        return;

    ExpendButton * btn = dynamic_cast<ExpendButton *>(btnsGroup->button(current));
    btn->click();
}

void KDSWidget::closeApp(){
    close();
}

void KDSWidget::receiveButtonClick(int x, int y){
//    qDebug() << "receive button press " << x << y;
    if (!this->geometry().contains(x, y)){
        close();
    }

}


void KDSWidget::msgReceiveAnotherOne(const QString &msg){
//    qDebug() << "another one " << msg;
    nextSelectedOption();
}

void KDSWidget::syncPrimaryScreenData(QString pName){
    if (!pName.isEmpty()){

        const KScreen::ConfigPtr &config = this->currentConfig();

        int scale = getCurrentScale();

        Q_FOREACH(const KScreen::OutputPtr &output, config->connectedOutputs()) {
            if (QString::compare(output.data()->name(), pName) == 0){
                QRect rect = output.data()->geometry();

                QDBusMessage message = QDBusMessage::createMethodCall("org.ukui.SettingsDaemon",
                                                       "/org/ukui/SettingsDaemon/wayland",
                                                       "org.ukui.SettingsDaemon.wayland",
                                                       "priScreenChanged");
                message << rect.x() / scale << rect.y() / scale << rect.width() / scale << rect.height() / scale << pName;
                QDBusConnection::sessionBus().send(message);
            }
        }
    }
}
