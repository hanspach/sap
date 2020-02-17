#include "deviceselectiondialog.h"
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QtNetwork>

int MainWindow::DELAY = 4000;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->fileButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    ui->folderButton->setIcon(style()->standardIcon(QStyle::SP_DirIcon));
    ui->radioButton->setIcon(QIcon(":/resources/station.png"));
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->speakerButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    statusLabel = new QLabel(this);
    statusLabel->setMinimumWidth(21);
    statusLabel->setAutoFillBackground(true);
    statusBar()->addPermanentWidget(statusLabel);
    ui->progressBar->setValue(0);
    player = new BassWrapper(this);
    model =  new TreeModel(this);
    ui->treeView->setModel(model);
    ui->treeView->setMouseTracking(true);   // needed for entered signal
    setAttribute(Qt::WA_AcceptTouchEvents, true);

    connect(ui->fileButton, SIGNAL(clicked()),this,SLOT(openFile()));
    connect(ui->folderButton, SIGNAL(clicked()),this,SLOT(openFolder()));
    connect(ui->radioButton, SIGNAL(clicked()),this,SLOT(inetRadio()));
    connect(ui->playButton, SIGNAL(clicked()),this,SLOT(playOrPause()));
    connect(ui->speakerButton, SIGNAL(clicked()),this,SLOT(chooseDevice()));
    connect(ui->treeView, SIGNAL(entered(QModelIndex)),this,SLOT(showItem(QModelIndex)));
    connect(ui->treeView->selectionModel(),SIGNAL(selectionChanged(const QItemSelection&,
        const QItemSelection&)),this,SLOT(updateActions(const QItemSelection&,const QItemSelection&)));
    connect(ui->treeView, SIGNAL(expanded(QModelIndex)),this, SLOT(nodeExpanded(QModelIndex)));
    connect(player, SIGNAL(metaData(QString)),this,SLOT(updateTitle(QString)));
    connect(player, SIGNAL(metaStatus(int,QString)),this,SLOT(updateStatus(int,QString)));
    connect(player, SIGNAL(mediaEnd()),this,SLOT(updateMediaEnd()));
    connect(player, SIGNAL(mediaLen(QString)),this,SLOT(updateMediaLen(QString)));
    connect(player, SIGNAL(mediaPos(QString,int)),this,SLOT(updateMediaPos(QString,int)));
    connect(ui->volumeSlider, SIGNAL(valueChanged(int)), this, SLOT(volumeChanged(int)));

    process   = new QProcess(this);
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(processFinished(int,QProcess::ExitStatus)));
    audioPath = QString();
    processName = "check";
    checkPulseAudio();
    QStringList pathes = QStandardPaths::standardLocations(QStandardPaths::MusicLocation);
    if(!pathes.isEmpty())
        audioPath = pathes.at(0);
    if(!testConnection()) {
        ui->radioButton->setEnabled(false);
        statusBar()->showMessage(tr("No internet access."),DELAY);
    }

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::openFile()
{
    bool stopped = false;
    if(player->state() == BassWrapper::PlayingState) {
        player->pause();
        stopped = true;
    }
    QFileDialog dlg(this);
    dlg.setFileMode(QFileDialog::ExistingFiles);
    dlg.setNameFilters(QStringList() << tr("Audio files (*.mp3 *.wav)"));
    dlg.setViewMode(QFileDialog::Detail);
    if(!audioPath.isEmpty())
        dlg.setDirectory(audioPath);
    if(dlg.exec()) {
        infolist.clear();
        foreach(QString filename, dlg.selectedFiles()) {
            infolist.append(QFileInfo(filename));
        }
        model->isNetMedia = false;
        player->isNetMedia= false;
        model->clear();
        audioPath = infolist.value(0).absolutePath();
        model->setupModelData(QFileInfoList() << audioPath);
        QTimer::singleShot(15000,this,SLOT(elapsedTime()));


    } else {
        statusBar()->showMessage(tr("File-open-dialog canceled."),DELAY);
        if(stopped) {
            player->play();
        }
    }
}

void MainWindow::openFolder()
{
    bool stopped = false;
    if(player->state() == BassWrapper::PlayingState) {
        player->pause();
        stopped = true;
    }
    QString folder = QFileDialog::getExistingDirectory(this, tr("Verzeichnis"),
        audioPath,QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if(!folder.isEmpty()) {
        QDir dir(folder);
        dir.setFilter(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks);
        infolist.clear();
        infolist = dir.entryInfoList();
        model->isNetMedia = false;
        player->isNetMedia= false;
        model->clear();
        audioPath = folder;
        model->setupModelData(QFileInfoList() << audioPath);
        QTimer::singleShot(15000,this,SLOT(elapsedTime()));
    } else {
        statusBar()->showMessage(tr("Folder-open-dialog canceled."),DELAY);
        if(stopped) {
            player->play();
        }
    }
}

void MainWindow::inetRadio()
{
    if(player->state() == BassWrapper::PlayingState) {
        player->pause();
    }
    QFile file(":/resources/bookmarks.xml");
    if(file.open(QIODevice::ReadOnly|QIODevice::Text)) {
        model->isNetMedia = true;
        player->isNetMedia= true;
        model->clear();
        model->setupModelData(file, "country");
        file.close();
    } else {
        updateStatus(BassWrapper::ErrorState, tr("Could not open..."));
    }
}

void MainWindow::chooseDevice()
{
    DeviceSelectionDialog dlg(player, this);
    if(player->state() == BassWrapper::PlayingState) {
        player->pause();
    }
    if(dlg.exec()) {
        QFile file("/usr/bin/pacmd");
        if(file.exists()) {
            process->start(file.fileName(),
                QStringList() << "set-default-sink" << dlg.deviceName());
        } else {
            if(dlg.activeDeviceNumber() != player->activeDevice()) {
                player->setActiveDevice(dlg.activeDeviceNumber());
            }
        }
    }
    else {
        if(player->state() == BassWrapper::PausedState)
            player->play();
    }
}

void MainWindow::playOrPause()
{
    if(ui->treeView->selectionModel() && !ui->treeView->selectionModel()->selection().isEmpty()) {
        if(player->state() == BassWrapper::PlayingState)
            player->pause();
        else if(player->state() == BassWrapper::PausedState)
            player->play();
    } else {
        statusBar()->showMessage(tr("To play a song: please expand the tree and select an entry."),DELAY);
    }
}

void MainWindow::nodeExpanded(const QModelIndex &node)
{
    if(node.isValid()) {
        if(player->state() == BassWrapper::PlayingState)
            player->pause();
        TreeModel* tnm = (TreeModel*)node.model();
        TreeItem*  tni = tnm->getItem(node);
        QModelIndex idx = node.child(0,0);
        if(idx.isValid()) {
            TreeModel* tim = (TreeModel*)idx.model();
            TreeItem*  tii = tim->getItem(idx);
            if(tni->childCount()==1 && tii->name().isEmpty()) {
                tim->removeRows(0,1, node);

                if(model->isNetMedia) {
                    QString layer = tni->iconPath() == ":/resources/station.png" ?
                        "bookmark" : "station";

                    QFile file(":/resources/bookmarks.xml");
                    if(file.open(QIODevice::ReadOnly|QIODevice::Text)) {
                        tnm->setupModelData(file,layer,tni->name(), node);
                        file.close();
                    }
                } else {
                    QFileInfoList list;
                    QDir dir(tni->url());
                    dir.setFilter(QDir::Dirs|QDir::Files|QDir::NoDotAndDotDot|QDir::NoSymLinks);
                    if(infolist.isEmpty()) {
                         foreach(QFileInfo info, dir.entryInfoList())
                             list << info;
                    } else {
                        foreach(QFileInfo info, infolist)
                            list << info;
                        infolist.clear();
                    }
                    tnm->setupModelData(list, node);
                }
            }
        }
    }
}

void MainWindow::updateActions(const QItemSelection& selected,
    const QItemSelection& /*deselected*/)
{
    QModelIndexList list = selected.indexes();
    if(!list.isEmpty()) {
         currentIndex = list.first();
         TreeItem* ti = ((TreeModel*)ui->treeView->model())->getItem(currentIndex);
         if(ti) {
             if(ti->childCount()) {
                 if(!ui->treeView->isExpanded(currentIndex))
                    ui->treeView->expand(currentIndex);
                 else
                     ui->treeView->collapse(currentIndex);
             } else {
                if(player->isNetMedia) {
                    ui->progressBar->setValue(0);
                    ui->lenLabel->setText("");
                    ui->posLabel->setText("");
                    player->prepareMedia(ti->url());
                }
                else
                    player->prepareFileStream(ti->url());
             }
         }
    }
}

void MainWindow::showItem(const QModelIndex& idx)
{
    TreeItem* ti = ((TreeModel*)ui->treeView->model())->getItem(idx);
    if(ti) {
        ui->treeView->setToolTip(ti->url());
    }
}

bool MainWindow::chooseTreeItem()
{
    QModelIndex idx = ui->treeView->indexBelow(currentIndex);
    if(idx.isValid()) {
        ui->treeView->selectionModel()->clearSelection();
        ui->treeView->selectionModel()->select(idx,QItemSelectionModel::Select);
        updateActions(ui->treeView->selectionModel()->selection());
        return true;
    }
    return false;
}

void MainWindow::volumeChanged(const int& volume)
{
   player->setVolume(volume);   //  vol-range 0..100
}

void MainWindow::updateMediaEnd()
{
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->progressBar->setValue(0);
    if(!chooseTreeItem()) {
         statusBar()->showMessage(tr("End of list reached."),DELAY);
    }
}

void MainWindow::elapsedTime()
{
    if(!ui->treeView->selectionModel()->hasSelection()) {
        TreeModel* tm = (TreeModel*)ui->treeView->model();
        QModelIndex idx = tm->index(0,0);
        if(idx.isValid()) {
            ui->treeView->expand(idx);
            idx = tm->index(0,0,idx);
            if(idx.isValid())
                ui->treeView->selectionModel()->select(idx,QItemSelectionModel::Select);
        }

    }
}

void MainWindow::updateMediaLen(const QString& len)
{
    ui->lenLabel->setText(len);
}

void MainWindow::updateMediaPos(const QString& pos, const int& percent)
{
    ui->posLabel->setText(pos);
    ui->progressBar->setValue(percent);
}

void MainWindow::updateTitle(const QString& title)
{
    statusBar()->showMessage(title);
}

void MainWindow::updateStatus(const int& status, const QString& reason)
{
    QPalette pal = statusLabel->palette();
    int state = status;
    if(state == BassWrapper::InitState)
        pal.setColor(QPalette::Window, QColor(Qt::yellow));
    else if(state == BassWrapper::PlayingState) {
        pal.setColor(QPalette::Window, QColor(Qt::green));
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
    }
    else if(state == BassWrapper::PausedState) {
        ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
         pal.setColor(QPalette::Window, QColor(Qt::lightGray));
    }
    else if(state == BassWrapper::ErrorState) {
        pal.setColor(QPalette::Window, QColor(Qt::red));
        statusBar()->showMessage(reason);
    }
    statusLabel->setPalette(pal);
}

void MainWindow::checkPulseAudio()
{
    QFile file("/usr/bin/pulseaudio");
    if(file.exists()) {
        processName = "check";
        process->start(file.fileName(),QStringList() << "--check");
    }
}

bool MainWindow::testConnection()
{
    foreach(QNetworkInterface iface, QNetworkInterface::allInterfaces()) {
        if(iface.flags().testFlag(QNetworkInterface::IsUp)
            && !iface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            return true;
        }
    }
    return false;
}

void MainWindow::processFinished(int exitCode,QProcess::ExitStatus exitState)
{
    if(processName == "check") {
        if(exitState == QProcess::NormalExit && exitCode !=0) {
            processName = "start";
            process->start("/usr/bin/pulseaudio", QStringList() << "--start");
        }
    }
    else if(processName == "start") {
        if(exitState == QProcess::NormalExit && exitCode == 0)
            statusBar()->showMessage(tr("Launched pulseaudio."),DELAY);
        else
            updateStatus(BassWrapper::ErrorState, tr("Failed to launch pulseaudio."));
    }
    else {
        if(exitState == QProcess::NormalExit && exitCode == 0) {
            statusBar()->showMessage(tr("Changed to ") + player->activeDevice(QFile()),DELAY);
        } else {
            updateStatus(BassWrapper::ErrorState, tr("Couldn't change the output-device"));
        }
   }
}
