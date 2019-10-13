#-------------------------------------------------
#
# Project created by QtCreator 2013-07-25T20:43:22
#
#-------------------------------------------------

QT       += core gui serialport concurrent
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets printsupport

requires(qtConfig(listwidget))
requires(qtConfig(combobox))

#include(../src/libQDeviceWatcher.pri)

TARGET = la-tests
TEMPLATE = app

# Tell the qcustomplot header that it will be used as library:
DEFINES += QCUSTOMPLOT_USE_LIBRARY

# Link with debug version of qcustomplot if compiling in debug mode, else with release library:
CONFIG(debug, release|debug) {
  win32:QCPLIB = qcustomplotd2
  else: QCPLIB = qcustomplotd
} else {
  win32:QCPLIB = qcustomplot2
  else: QCPLIB = qcustomplot
}

CONFIG(debug, release|debug) {
  win32:QDWLIB = QDeviceWatcherd2
  else: QDWLIB = QDeviceWatcherd2
} else {
  win32:QDWLIB = QDeviceWatcher2
  else: QDWLIB = QDeviceWatcher2
}
LIBS += -L./ -l../$$QCPLIB

#LIBS += -L./ -l../$$QDWLIB

SOURCES += main.cpp\
    intelhexclass.cpp \
    load-ice40.cpp \
    load-image.cpp \
    loader-ds30loader.cpp \
    loader-intelhex.cpp \
        mainwindow.cpp\
    console.cpp

HEADERS  += mainwindow.h\
    intelhexclass.h \
    console.h \
    load-ice40.h \
    load-image.h \
    loader-ds30loader.h \
    loader-intelhex.h

RESOURCES += dockwidgets.qrc\
            terminal.qrc

FORMS +=
