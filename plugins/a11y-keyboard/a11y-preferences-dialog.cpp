#include "a11y-preferences-dialog.h"
#include "ui_a11y-preferences-dialog.h"

A11yPreferencesDialog::A11yPreferencesDialog(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::A11yPreferencesDialog)
{
    ui->setupUi(this);
}

A11yPreferencesDialog::~A11yPreferencesDialog()
{
    delete ui;
}

void A11yPreferencesDialog::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    USD_LOG(LOG_DEBUG,"dialog had close");
    Q_EMIT singalCloseWidget();
}
