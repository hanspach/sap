#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QFileInfo>
#include "treemodel.h"
#include "basswrapper.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void openFile();
    void openFolder();
    void inetRadio();
    void chooseDevice();
    void nodeExpanded(const QModelIndex &node);
    void updateActions(const QItemSelection& selected,const QItemSelection& deselected=QItemSelection());
    void showItem(const QModelIndex& idx);
    void volumeChanged(const int& volume);
    void playOrPause();
    void elapsedTime();
    void updateStatus(const int& status, const QString& reason);
    void updateTitle(const QString& title);
    void updateMediaPos(const QString& pos, const int& percent);
    void updateMediaLen(const QString& len);
    void updateMediaEnd();
private:
    Ui::MainWindow *ui;
    static int DELAY;
    QFileInfoList infolist;
    QLabel* statusLabel;
    TreeModel* model;
    BassWrapper* player;
    QString audioPath;
    QModelIndex currentIndex;

    bool chooseTreeItem();
};

#endif // MAINWINDOW_H
