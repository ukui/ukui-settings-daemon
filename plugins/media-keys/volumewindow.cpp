#include "volumewindow.h"
#include "ui_volumewindow.h"
#include <QPalette>
#include <QSize>
#include <QRect>
#include <QDebug>

const QString ICONDIR = "/usr/share/icons/ukui-icon-theme-default/scalable";

const QString allIconName[] = {
    ICONDIR + "/status/audio-volume-muted.svg",         //0
    ICONDIR + "/status/audio-volume-low.svg",
    ICONDIR + "/status/audio-volume-medium.svg",
    ICONDIR + "/status/audio-volume-high.svg",
    nullptr
};

VolumeWindow::VolumeWindow(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::VolumeWindow)
{
    ui->setupUi(this);
}

VolumeWindow::~VolumeWindow()
{
    delete ui;
    delete mVLayout;
    delete mBarLayout;
    delete mSvgLayout;
    delete mSvg;
    delete mBar;
    delete mTimer;
}

void VolumeWindow::initWindowInfo()
{
    //窗口性质
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setWindowOpacity(0.95);          //设置透明度
    setPalette(QPalette(Qt::black));//设置窗口背景色
    setAutoFillBackground(true);

    //new memery
    mVLayout = new QVBoxLayout(this);
    mBarLayout = new QHBoxLayout();
    mSvgLayout = new QHBoxLayout();
    mBar = new QProgressBar();
    mSvg = new QSvgWidget();
    mTimer = new QTimer();
    connect(mTimer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));

    mVolumeLevel = 0;
    mVolumeMuted = false;
    setWidgetLayout();
}

//上下留出10个空间,音量条与svg图片之间留出10个空间
void VolumeWindow::setWidgetLayout()
{
    //窗口性质
    setFixedSize(QSize(64,300));

    //svg图片操作
    mSvg->setFixedSize(QSize(32,32));

    //音量条操作
    mBar->setOrientation(Qt::Vertical);
    mBar->setFixedSize(QSize(10,230));
    mBar->setTextVisible(false);
//  mBar->setValue(volumeLevel/100);
    mBar->setStyleSheet("QProgressBar{border:none;border-radius:5px;background:#708069}"
                       "QProgressBar::chunk{border-radius:5px;background:white}");

    //音量调放入横向布局
    mBarLayout->addWidget(mBar);
    mBarLayout->setContentsMargins(0,0,0,15);

    //svg图片加到横向布局
    mSvgLayout->addWidget(mSvg);

    //横向布局和svg图片加入垂直布局
    mVLayout->addLayout(mBarLayout);
    mVLayout->addLayout(mSvgLayout);
    mVLayout->setGeometry(QRect(0,0,width(),height()));
}

void VolumeWindow::dialogShow()
{
    mSvg->load(mIconName);
    show();
    mTimer->start(2000);
}

void VolumeWindow::setVolumeMuted(bool muted)
{
    if(this->mVolumeMuted != muted)
        mVolumeMuted = muted;
}

void VolumeWindow::setVolumeLevel(int level)
{
    double percentage;

    this->mVolumeLevel = level;
    mBar->setValue((mVolumeLevel-mMinVolume)/100);
    mIconName.clear();

    if(mVolumeMuted){
        mIconName = allIconName[0];
        return;
    }

    percentage = double(mVolumeLevel-mMinVolume) / (mMaxVolume-mMinVolume);

    if(0 <= percentage && percentage <= 0.01)
        mIconName = allIconName[0];
    if(percentage <= 0.33)
        mIconName = allIconName[1];
    else if(percentage <= 0.66)
        mIconName = allIconName[2];
    else
        mIconName = allIconName[3];
    //qDebug()<<percentage<<" "<<(double)50096/65536<<" "<<mIconName<<endl;
}

void VolumeWindow::setVolumeRange(int min, int max)
{
    if(min == mMinVolume && max == mMaxVolume)
        return ;

    mMaxVolume = max;
    mMinVolume = min;
    mBar->setRange(min,(max-min)/100);
}

void VolumeWindow::timeoutHandle()
{
    hide();
    mTimer->stop();
}
