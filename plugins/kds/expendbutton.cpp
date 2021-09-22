#include "expendbutton.h"

#include <QDebug>
#include <QGraphicsBlurEffect>

#define QT_THEME_SCHEMA             "org.ukui.style"



ExpendButton::ExpendButton(QWidget *parent) :
    QPushButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);
    setAttribute(Qt::WA_TranslucentBackground, true);


    m_styleSettings = new QGSettings(QT_THEME_SCHEMA);

    sign = 0;

    QHBoxLayout * generalHorLayout = new QHBoxLayout(this);
    generalHorLayout->setSpacing(0);
    generalHorLayout->setContentsMargins(44, 0, 20, 0);

    logoLabel = new QLabel(this);
    logoLabel->setFixedSize(QSize(60, 60));
    textLabel = new QLabel(this);

    QSizePolicy textSizePolicy = textLabel->sizePolicy();
    textSizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
    textSizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    textLabel->setSizePolicy(textSizePolicy);

    spaceLabel = new QLabel(this);
    spaceLabel->setFixedSize(QSize(24, 30));

    statusLabel = new QLabel(this);
    statusLabel->setFixedSize(QSize(27, 18));


    generalHorLayout->addWidget(logoLabel, Qt::AlignVCenter);
    generalHorLayout->addWidget(spaceLabel, Qt::AlignVCenter);
    generalHorLayout->addWidget(textLabel, Qt::AlignVCenter);
    generalHorLayout->addStretch();
    generalHorLayout->addWidget(statusLabel, Qt::AlignVCenter);

    setLayout(generalHorLayout);

}

ExpendButton::~ExpendButton()
{
}

void ExpendButton::setSign(int id ,const QString &style){
    if(style == "ukui-light")
    {
        qss1 = QString("QPushButton{background: #00F5F5F5; border: none;}"\
                       "QPushButton:hover{background: #40000000; border: none;}"\
                       "QPushButton:checked{background: #40000000; border: none;}");

        qss0 = QString("QPushButton{background: #0D000000; border: none;}"\
                       "QPushButton:hover{background: #40000000; border: none;}"\
                       "QPushButton:checked{background: #40000000; border: none;}");
        textLabel->setStyleSheet("color: #262626;");


    }else
    {
        qss0 = QString("QPushButton{background: #0DFFFFFF; border: none;}"\
                       "QPushButton:hover{background: #40F5F5F5; border: none;}"\
                       "QPushButton:checked{background: #40F5F5F5; border: none;}");

        qss1 = QString("QPushButton{background: #00232426; border: none;}"\
                       "QPushButton:hover{background: #40F5F5F5; border: none;}"\
                       "QPushButton:checked{background: #40F5F5F5; border: none;}");
        textLabel->setStyleSheet("color: #FFFFFF;");
    }
    statusLabel->setPixmap(drawLightColoredPixmap(QPixmap(":/img/selected.png"),style));

    sign = id;
    if (sign == 0){
        setStyleSheet(qss0);
    } else if (sign == 1){
        setStyleSheet(qss1);
    }
}

void ExpendButton::setBtnLogo(QString logo,const QString &style){
    logoLabel->setPixmap(drawLightColoredPixmap(QPixmap(logo),style));
}

void ExpendButton::setBtnText(QString text){
    textLabel->setText(text);
}

void ExpendButton::setBtnChecked(bool checked){
    if (checked){
        statusLabel->show();
    } else {
        statusLabel->hide();
    }
}

bool ExpendButton::getBtnChecked(){
    return statusLabel->isVisible();
}

QPixmap ExpendButton::drawLightColoredPixmap(const QPixmap &source, const QString &style)
{
    int value = 255;
    if(style == "ukui-light")
    {
        value = 0;
    }

    QColor gray(255,255,255);
    QImage img = source.toImage();
    for (int x = 0; x < img.width(); x++) {
        for (int y = 0; y < img.height(); y++) {
            auto color = img.pixelColor(x, y);
            if (color.alpha() > 0) {
                if (qAbs(color.red()-gray.red())<20 && qAbs(color.green()-gray.green())<20 && qAbs(color.blue()-gray.blue())<20) {
                    color.setRed(value);
                    color.setGreen(value);
                    color.setBlue(value);
                    img.setPixelColor(x, y, color);
                }
                else {
                    color.setRed(value);
                    color.setGreen(value);
                    color.setBlue(value);
                    img.setPixelColor(x, y, color);
                }
            }
        }
    }
    return QPixmap::fromImage(img);
}
