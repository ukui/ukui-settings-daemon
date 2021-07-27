#ifndef A11YPREFERENCESDIALOG_H
#define A11YPREFERENCESDIALOG_H

#include <QWidget>
#include "clib-syslog.h"


namespace Ui {
class A11yPreferencesDialog;
}

class A11yPreferencesDialog : public QWidget
{
    Q_OBJECT

public:
    explicit A11yPreferencesDialog(QWidget *parent = nullptr);
    ~A11yPreferencesDialog();

Q_SIGNALS:

    void singalCloseWidget();

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::A11yPreferencesDialog *ui;



};

#endif // A11YPREFERENCESDIALOG_H
