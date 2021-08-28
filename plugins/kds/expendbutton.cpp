#include "expendbutton.h"

#include <QDebug>


ExpendButton::ExpendButton(QWidget *parent) :
    QPushButton(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setCheckable(true);

    sign = 0;

    qss0 = QString("QPushButton{background: #99000000; border: none;}"
                   "QPushButton:hover{background: #80000000; border: none;}"
                   "QPushButton:checked{background: #80000000; border: none;}"
                   "/*QPushButton:!checked{background: #99000000; border: none;}*/");
    qss1 = QString("QPushButton{background: #A6000000; border: none;}"
                   "QPushButton:hover{background: #80000000; border: none;}"
                   "QPushButton:checked{background: #80000000; border: none;}"
                   "/*QPushButton:!checked{background: #A6000000; border: none;}*/");

    QHBoxLayout * generalHorLayout = new QHBoxLayout(this);
    generalHorLayout->setSpacing(0);
    generalHorLayout->setContentsMargins(44, 0, 20, 0);

    logoLabel = new QLabel(this);
    logoLabel->setFixedSize(QSize(60, 60));
    textLabel = new QLabel(this);
    textLabel->setStyleSheet("QLabel{color: #FFFFFF;}");
    QSizePolicy textSizePolicy = textLabel->sizePolicy();
    textSizePolicy.setHorizontalPolicy(QSizePolicy::Fixed);
    textSizePolicy.setVerticalPolicy(QSizePolicy::Fixed);
    textLabel->setSizePolicy(textSizePolicy);

    spaceLabel = new QLabel(this);
    spaceLabel->setFixedSize(QSize(68, 30));

    statusLabel = new QLabel(this);
    statusLabel->setFixedSize(QSize(27, 18));
    statusLabel->setPixmap(QPixmap(":/img/selected.png"));


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

void ExpendButton::setSign(int id){
    sign = id;

    if (sign == 0){
        setStyleSheet(qss0);
    } else if (sign == 1){
        setStyleSheet(qss1);
    }
}

void ExpendButton::setBtnLogo(QString logo){
    logoLabel->setPixmap(QPixmap(logo));
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
