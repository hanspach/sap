#ifndef DEVICESELECTIONDIALOG_H
#define DEVICESELECTIONDIALOG_H

#include <QDialog>

namespace Ui {
class DeviceSelectionDialog;
}

class DeviceSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceSelectionDialog(QWidget *parent = 0);
    ~DeviceSelectionDialog();

private:
    Ui::DeviceSelectionDialog *ui;
};

#endif // DEVICESELECTIONDIALOG_H
