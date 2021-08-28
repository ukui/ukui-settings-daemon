#ifndef KEYBOARDWIDGET_H
#define KEYBOARDWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPainter>


namespace Ui {
class KeyboardWidget;
}

class KeyboardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit KeyboardWidget(QWidget *parent = nullptr);
    ~KeyboardWidget();

    void setIcons(QString icon);
    void showWidget();



private:
    void initWidgetInfo();
    void geometryChangedHandle();

public Q_SLOTS:
    void timeoutHandle();

private:
    Ui::KeyboardWidget *ui;

    QString          m_iconName;

    QPushButton      *m_btnStatus;
    QTimer           *m_timer;
};

#endif // KEYBOARDWIDGET_H
