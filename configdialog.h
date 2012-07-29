#ifndef CONFIGDIALOG_H
#define CONFIGDIALOG_H

#include <QDialog>

namespace Ui {
    class configDialog;
}

class configDialog : public QDialog
{
    Q_OBJECT

public:
    explicit configDialog(QWidget *parent = 0);
    ~configDialog();

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_buttonBox_accepted();

private:
    Ui::configDialog *ui;
};

#endif // CONFIGDIALOG_H
