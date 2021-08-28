#include "keyboard-widget.h"
#include "ui_keyboardwidget.h"
#include <QTimer>
#include <QPainter>
#include <QBitmap>
#include <QScreen>


KeyboardWidget::KeyboardWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::KeyboardWidget)
{
    ui->setupUi(this);
    initWidgetInfo();
}

KeyboardWidget::~KeyboardWidget()
{
    delete ui;
}

void KeyboardWidget::initWidgetInfo()
{
    m_timer = new QTimer(this);
    connect(m_timer,SIGNAL(timeout()),this,SLOT(timeoutHandle()));

    m_btnStatus = new QPushButton(this);
    m_btnStatus->setDisabled(true);

    connect(QApplication::primaryScreen(), &QScreen::geometryChanged, this, &KeyboardWidget::geometryChangedHandle);

    connect(static_cast<QApplication *>(QCoreApplication::instance()),
            &QApplication::primaryScreenChanged, this, &KeyboardWidget::geometryChangedHandle);

    setFixedSize(72,72);
    setWindowFlags(Qt::FramelessWindowHint |
                   Qt::Tool |
                   Qt::WindowStaysOnTopHint |
                   Qt::X11BypassWindowManagerHint |
                   Qt::Popup);

    QBitmap bmp(this->size());
    bmp.fill();
    QPainter p(&bmp);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#232426"));
    p.setRenderHint(QPainter::Antialiasing);
    p.drawRoundedRect(bmp.rect(),10,10);
    setMask(bmp);
    m_btnStatus->setMask(bmp);



    setPalette(QPalette(QColor("#232426")));//设置窗口背景色
    setAutoFillBackground(true);
    geometryChangedHandle();
}


void KeyboardWidget::timeoutHandle()
{
    m_timer->stop();
    hide();
}
void KeyboardWidget::showWidget()
{
    m_btnStatus->setFixedSize(this->size());
    m_btnStatus->setIconSize(QSize(48,48));

    m_btnStatus->setIcon(QIcon::fromTheme(m_iconName,QIcon("")));
    show();
    m_timer->start(2500);

}


void KeyboardWidget::setIcons(QString icon)
{
    m_iconName = icon;
}

void KeyboardWidget::geometryChangedHandle()
{
    int x=QApplication::primaryScreen()->geometry().x();
    int y=QApplication::primaryScreen()->geometry().y();
    int width = QApplication::primaryScreen()->size().width();
    int height = QApplication::primaryScreen()->size().height();

    int ax,ay;
    ax = x+width - this->width() - 200;
    ay = y+height - this->height() - 50;
    move(ax,ay);
}


