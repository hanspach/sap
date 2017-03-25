#include "deviceselectiondialog.h"
#include "ui_deviceselectiondialog.h"

DeviceSelectionDialog::DeviceSelectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DeviceSelectionDialog)
{
    ui->setupUi(this);
}

DeviceSelectionDialog::~DeviceSelectionDialog()
{
    delete ui;
}
