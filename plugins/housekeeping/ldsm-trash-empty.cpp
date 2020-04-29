#include "ldsm-trash-empty.h"
#include "ui_ldsm-trash-empty.h"

#include <QIcon>
#include <QFile>
#include <QDebug>

#include <glib.h>
#include <QGSettings/qgsettings.h>

LdsmTrashEmpty::LdsmTrashEmpty(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LdsmTrashEmpty)
{
    ui->setupUi(this);
    windowLayoutInit();
}

LdsmTrashEmpty::~LdsmTrashEmpty()
{
    delete ui;
}

void LdsmTrashEmpty::windowLayoutInit()
{
    setFixedSize(400,200);
    setWindowTitle(tr("Emptying the trash"));
    setWindowIcon(QIcon("/new/prefix1/warning.png"));

}
