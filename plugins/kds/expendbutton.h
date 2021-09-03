#ifndef EXPENDBUTTON_H
#define EXPENDBUTTON_H


#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

class ExpendButton : public QPushButton
{
    Q_OBJECT

public:
    explicit ExpendButton(QWidget *parent = 0);
    ~ExpendButton();

public:
    void setSign(int id ,const QString &style = "");
    void setBtnLogo(QString logo,const QString &style = "");
    void setBtnText(QString text);
    void setBtnChecked(bool checked);
    bool getBtnChecked();

private:
    QPixmap drawLightColoredPixmap(const QPixmap &source, const QString &style);


private:
    QLabel * logoLabel;
    QLabel * textLabel;
    QLabel * spaceLabel;
    QLabel * statusLabel;

    int sign;

    QString qss0;
    QString qss1;

};

#endif // EXPENDBUTTON_H
