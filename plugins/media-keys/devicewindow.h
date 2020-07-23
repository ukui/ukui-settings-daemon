#ifndef DEVICEWINDOW_H
#define DEVICEWINDOW_H

#include <QWidget>
#include <QString>
#include <QSvgWidget>
#include <QApplication>
#include <QX11Info>
#include <QScreen>
#include <QTimer>

namespace Ui {
class DeviceWindow;
}

class DeviceWindow : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceWindow(QWidget *parent = nullptr);
    ~DeviceWindow();
    void initWindowInfo();
    void setAction(const QString);
    void dialogShow();

private:
    void ensureSvgInfo(int*,int*,int*,int*);

private Q_SLOTS:
    void timeoutHandle();

private:
    Ui::DeviceWindow *ui;
    QString          mIconName;
    QSvgWidget       *mSvg;
    QTimer           *mTimer;
};

#endif // DEVICEWINDOW_H
