#ifndef A11YPREFERENCESDIALOG_H
#define A11YPREFERENCESDIALOG_H

#include <QWidget>

namespace Ui {
class A11yPreferencesDialog;
}

class A11yPreferencesDialog : public QWidget
{
    Q_OBJECT

public:
    explicit A11yPreferencesDialog(QWidget *parent = nullptr);
    ~A11yPreferencesDialog();

private:
    Ui::A11yPreferencesDialog *ui;
};

#endif // A11YPREFERENCESDIALOG_H
