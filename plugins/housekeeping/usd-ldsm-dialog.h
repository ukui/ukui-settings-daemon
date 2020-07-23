#ifndef USDLDSMDIALOG_H
#define USDLDSMDIALOG_H


#include <QDialog>
#include <QString>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>

#define SETTINGS_SCHEMA "org.ukui.SettingsDaemon.plugins.housekeeping"
#define SETTINGS_IGNORE_PATHS "ignore-paths"

#define LDSM_DIALOG_IGNORE                  10
#define LDSM_DIALOG_RESPONSE_ANALYZE        30
#define LDSM_DIALOG_RESPONSE_EMPTY_TRASH    40

QT_BEGIN_NAMESPACE
namespace Ui { class LdsmDialog; }
QT_END_NAMESPACE

class LdsmDialog : public QDialog
{
    Q_OBJECT

public:
    LdsmDialog(QWidget *parent = nullptr);
    LdsmDialog(bool other_usable_partitions,bool other_partitions,bool display_baobab,bool has_trash,
               long space_remaining,QString  partition_name,QString mount_path,
               QWidget *parent = nullptr);
    ~LdsmDialog();

private:
    Ui::LdsmDialog *ui;
    QLabel* picture_label;
    QLabel* primary_label;
    QLabel* second_label;
    QCheckBox* ignore_check_button;
    QPushButton* trash_empty;
    QPushButton* ignore_button;
    QPushButton* analyze_button;


    bool other_usable_partitions;
    bool other_partitions;
    bool has_trash;
    long space_remaining;
    /*char *partition_name;
    char *mount_path;*/
    QString partition_name;
    QString mount_path;

public Q_SLOTS:
    void checkButtonClicked(int);
    void checkButtonIgnore ();
    void checkButtonAnalyze ();
    void checkButtonTrashEmpty();

private:
    void windowLayoutInit(bool display_baobab);
    QString getPrimaryText();
    QString getSecondText();
    QString getCheckButtonText();
    void allConnectEvent(bool display_baobab);

};

#endif // USDLDSMDIALOG_H
