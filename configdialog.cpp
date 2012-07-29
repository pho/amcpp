#include "configdialog.h"
#include "ui_configdialog.h"
#include <QSettings>

configDialog::configDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::configDialog)
{
    ui->setupUi(this);
    QSettings settings("amcpp", "amcpp");

    ui->amUrlLine->setText(settings.value("amUrl").toString());
    ui->userLine->setText(settings.value("username").toString());
    ui->strmPassLine->setText(settings.value("streamPass").toString());
}

configDialog::~configDialog()
{
    delete ui;
}

void configDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void configDialog::on_buttonBox_accepted()
{
    QSettings settings("amcpp", "amcpp");
    settings.setValue("amUrl", ui->amUrlLine->text());
    settings.setValue("username", ui->userLine->text());
    settings.setValue("streamPass", ui->strmPassLine->text());
}
