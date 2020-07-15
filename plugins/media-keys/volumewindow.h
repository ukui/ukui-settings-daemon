#ifndef MEDIAKEYSWINDOW_H
#define MEDIAKEYSWINDOW_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QProgressBar>
#include <QSvgWidget>
#include <QTimer>
#include <QString>

QT_BEGIN_NAMESPACE
namespace Ui {class VolumeWindow;}
QT_END_NAMESPACE

class VolumeWindow : public QWidget
{
    Q_OBJECT

public:
    VolumeWindow(QWidget *parent = nullptr);
    ~VolumeWindow();
    void initWindowInfo();
    void dialogShow();
    void setWidgetLayout();
    void setVolumeMuted(bool);
    void setVolumeLevel(int);
    void setVolumeRange(int, int);

private Q_SLOTS:
    void timeoutHandle();

private:
    Ui::VolumeWindow *ui;
    QVBoxLayout *mVLayout;
    QHBoxLayout *mBarLayout;
    QHBoxLayout *mSvgLayout;
    QSpacerItem *mSpace;
    QProgressBar *mBar;
    QSvgWidget   *mSvg;
    QTimer       *mTimer;
    QString      mIconName;

    int mVolumeLevel;
    int mMaxVolume,mMinVolume;
    bool mVolumeMuted;
};
#endif // MEDIAKEYSWINDOW_H
