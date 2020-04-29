#ifndef LDSMTRASHEMPTY_H
#define LDSMTRASHEMPTY_H

#include <QObject>
#include <QDialog>
#include <QLabel>
#include <QFile>
#include <QProgressBar>
#include <QPushButton>
#include <QMessageBox>

namespace Ui {
class LdsmTrashEmpty;
}

class LdsmTrashEmpty : public QDialog
{
    Q_OBJECT

public:
    explicit LdsmTrashEmpty(QWidget *parent = nullptr);
    ~LdsmTrashEmpty();
    void trashEmptyStart();
    void showConfirmationDialog();

private:
    Ui::LdsmTrashEmpty *ui;


private:
    void windowLayoutInit();


};

#endif // LDSMTRASHEMPTY_H
