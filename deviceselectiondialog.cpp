#include "deviceselectiondialog.h"
#include "ui_deviceselectiondialog.h"

DeviceSelectionDialog::DeviceSelectionDialog(BassWrapper *ctrl, QWidget *parent) :
    QDialog(parent), ui(new Ui::DeviceSelectionDialog)
{
    ui->setupUi(this);
    sink = QString();
    player = ctrl;
    Qt::WindowFlags flags = windowFlags();  // remove the help flag
    Qt::WindowFlags helpFlag = Qt::WindowContextHelpButtonHint;
    flags = flags & (~helpFlag);
    setWindowFlags(flags);

    int y = 7;
    QList<DeviceInfo> infos;
    QFile file("/usr/bin/pacmd");
    if(file.exists()) {
        infos = player->devices(file);
        for(int i=0; i < infos.length(); ++i) {
            qDebug() << "Bez:" << infos.at(i).name << " Sink:" << infos.at(i).device << " isActive:" << QString::number(infos.at(i).active);
            if(infos.at(i).active) {
                activeDevice = i + 1;
                break;
            }
        }
    } else {
        activeDevice = player->device();
        infos = player->devices();
    }
    for(int i=0; i < infos.length(); ++i) {
        QRadioButton* b = new QRadioButton(infos.at(i).name);
        b->setGeometry(5,y,271,13);
        if(i+1 == activeDevice) {
            b->setChecked(true);
        }
        ui->verticalLayout->addWidget(b);
        buttons.append(b);
        y += 23;
    }
}

DeviceSelectionDialog::~DeviceSelectionDialog()
{
    delete ui;
}

int DeviceSelectionDialog::activeDeviceNumber() const
{
    return activeDevice;
}

void DeviceSelectionDialog::accept()
{
    for(int i=0; i < buttons.length(); ++i) {
       if(buttons.at(i)->isChecked()) {
           sink = player->getDevicesInfo().at(i).device;
           activeDevice = i+1;
           break;
       }
   }
   QDialog::accept();
}

QString DeviceSelectionDialog::deviceName() const
{
    return sink;
}
