QT      += core gui widgets webkitwidgets
TARGET = sap
TEMPLATE = app
SOURCES += main.cpp\
        mainwindow.cpp \
    treeitem.cpp \
    treemodel.cpp \
    basswrapper.cpp \
    deviceselectiondialog.cpp

HEADERS += mainwindow.h \
    bass.h \
    treeitem.h \
    treemodel.h \
    basswrapper.h \
    deviceselectiondialog.h

FORMS   += mainwindow.ui \
    deviceselectiondialog.ui
LIBS    += -L$$PWD/ -lbass

RESOURCES += \
    sap.qrc
