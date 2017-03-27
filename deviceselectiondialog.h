#ifndef DEVICESELECTIONDIALOG_H
#define DEVICESELECTIONDIALOG_H

#include <QDialog>
#include <QList>
#include "basswrapper.h"

QT_BEGIN_NAMESPACE
class QRadioButton;
QT_END_NAMESPACE

namespace Ui {
class DeviceSelectionDialog;
}

class DeviceSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceSelectionDialog(BassWrapper* ctrl, QWidget *parent=0);
    ~DeviceSelectionDialog();
    int activeDeviceNumber() const;
    QString deviceName() const;
    void accept();
private:
    Ui::DeviceSelectionDialog *ui;
    BassWrapper* player;
    int activeDevice;
    QString sink;
    QList<QRadioButton*> buttons;
};

#endif // DEVICESELECTIONDIALOG_H
