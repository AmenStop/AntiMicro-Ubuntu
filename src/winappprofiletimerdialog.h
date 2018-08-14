#ifndef WINAPPPROFILETIMERDIALOG_H
#define WINAPPPROFILETIMERDIALOG_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class WinAppProfileTimerDialog;
}

class WinAppProfileTimerDialog : public QDialog
{
    Q_OBJECT

public:
    explicit WinAppProfileTimerDialog(QWidget *parent = nullptr);
    ~WinAppProfileTimerDialog();

protected:
    QTimer appTimer;

private:
    Ui::WinAppProfileTimerDialog *ui;

private slots:
    void startTimer();
};

#endif // WINAPPPROFILETIMERDIALOG_H
