#ifndef KEYBOARDWIDGET_H
#define KEYBOARDWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QPainter>
#include <QGSettings/qgsettings.h>



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

protected:
    void paintEvent(QPaintEvent *event);
    void showEvent(QShowEvent* event);
    void resizeEvent(QResizeEvent* even);





private:
    void initWidgetInfo();
    void geometryChangedHandle();
    QPixmap drawLightColoredPixmap(const QPixmap &source, const QString &style);


public Q_SLOTS:
    void timeoutHandle();
    void onStyleChanged(const QString&);

private:
    Ui::KeyboardWidget *ui;
    QWidget*         m_backgroudWidget;
    QString          m_iconName;

    QLabel      *m_btnStatus;
    QTimer           *m_timer;

    QGSettings       *m_styleSettings;
};

#endif // KEYBOARDWIDGET_H
