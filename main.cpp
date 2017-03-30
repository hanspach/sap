#include "mainwindow.h"
#include <QtWidgets>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QRect r = QApplication::desktop()->screenGeometry();
    int   x = (r.width() - w.width()) / 2;
    int   y = (r.height()- w.height()) / 2;
    w.move(x,y);
    w.show();

    return a.exec();
}
