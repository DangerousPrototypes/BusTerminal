/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/
/*
Bus Terminal is built on Qt creator examples (Console, Docker) and QCustomPlot examples

Qt is available under the GNU General Public License version 3.
The Qt Toolkit is Copyright (C) 2018 The Qt Company Ltd. and other contributors.
Contact: https://www.qt.io/licensing/

QCustomPlot is available under the GNU General Public License version 3.
QCustomPlot is Copyright (C) 2019 Emanuel Eichhammer
Contact: https://www.qcustomplot.com/index.php/introduction

INTEL HEX FILE CLASS MODULE is Copyright (c) 2012 Stuart Cording
Contact: http://codinghead.github.com/Intel-HEX-Class
*/

#include <QtWidgets>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printdialog)
#include <QtPrintSupport>
#endif
#endif

#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QSerialPortInfo>
#include <QDebug>

//#include <stdlib.h>

#include "mainwindow.h"
#include "console.h"
//#include "intelhexclass.h"
#include "loader-ds30loader.h"
#include "loader-intelhex.h"
#include "load-ice40.h"

static const char blankString[] = QT_TRANSLATE_NOOP("SettingsDialog", "N/A");

MainWindow::MainWindow():
	m_status(new QLabel),
	m_console(new Console),
	//m_settings(new SettingsDialog),
    m_serial(new QSerialPort(this)),
    m_serialBinaryMode(new QSerialPort(this))
{
	setWindowTitle(tr("Bus Terminal"));
	resize(1200, 600);
    m_bsComplete=true;
    m_logicAnalyzerSamplesExpected=0;

	//m_ui->setupUi(this);
	m_console->setEnabled(false);
	m_console->disconnected();
	setCentralWidget(m_console);

	//m_ui->actionConnect->setEnabled(true);
	//m_ui->actionDisconnect->setEnabled(false);
	//m_ui->actionQuit->setEnabled(true);
	//m_ui->actionConfigure->setEnabled(true);
	//m_ui->statusBar->addWidget(m_status);

	connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
	connect(m_serial, &QSerialPort::readyRead, this, &MainWindow::readData);
	connect(m_console, &Console::getData, this, &MainWindow::writeData);
	//connect(m_serial, &QSerialPort::errorOccurred, this, &MainWindow::fillPortsInfo);

    connect(m_serialBinaryMode, &QSerialPort::errorOccurred, this, &MainWindow::handleError);
    connect(m_serialBinaryMode, &QSerialPort::readyRead, this, &MainWindow::binaryModeReadData);


    createActions();
	createStatusBar();
	createDockWindows();
	setUnifiedTitleAndToolBarOnMac(true);

	// configure scroll bars:
	// Since scroll bars only support integer values, we'll set a high default range of -500..500 and
	// divide scroll bar position values by 100 to provide a scroll range -5..5 in floating point
	// axis coordinates. if you want to dynamically grow the range accessible with the scroll bar,
	// just increase the the minimum/maximum values of the scroll bars as needed.
	//ui->horizontalScrollBar->setRange(0, 500);

	// create connection between axes and scroll bars
	//connect(ui->horizontalScrollBar, SIGNAL(valueChanged(int)), this, SLOT(horzScrollBarChanged(int)));
	connect(m_customPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(lockRange(QCPRange)));

	// rescale to fit the data
	m_customPlot->xAxis->rescale(true);

	QObject::connect(conv_dec, &QLineEdit::textEdited, this, &MainWindow::convert);
	QObject::connect(conv_hex, &QLineEdit::textEdited, this, &MainWindow::convert);
	QObject::connect(conv_bin, &QLineEdit::textEdited, this, &MainWindow::convert);
	QObject::connect(conv_ascii, &QLineEdit::textEdited, this, &MainWindow::convert);

	connect(serialPortInfoListBox, QOverload<int>::of(&QComboBox::currentIndexChanged),this, &MainWindow::showPortInfo);
	fillPortsInfo();

	/*
	serialPortWatcher = new QDeviceWatcher;
	serialPortWatcher->appendEventReceiver(this);
	connect( serialPortWatcher, SIGNAL(deviceAdded(QString)), this, SLOT(fillPortsInfo()));
	connect( serialPortWatcher, SIGNAL(deviceChanged(QString)), this, SLOT(fillPortsInfo()));
	connect( serialPortWatcher, SIGNAL(deviceRemoved(QString)), this, SLOT(fillPortsInfo()));
	*/

	periodicTimer = new QTimer(this);
	connect(periodicTimer, &QTimer::timeout, this, &MainWindow::periodicEvents);
	periodicTimer->start(500);

	/*
    // Initialize vector with strings using push_back  
    // command 
    SerialPorts.push_back("Blue"); 
    colour.push_back("White"); 
    colour.push_back("Black"); 
    colour.push_back("Red"); 
    colour.push_back("Green"); 

    // Print Strings stored in Vector 
	qDebug() << colour.size();
	//for(int i = 0; i < colour.size(); i++)     
		//qDebug() << colour[i];
	*/

	showMaximized();
}

void MainWindow::periodicEvents()
{
	//qDebug() << "periodicEvents";

	//this file watching could defo be replaced with QFileSystemWatcher....
	if(!curBitstreamFile.isEmpty() && m_bsUpdateAutoCheckBox->isChecked())
	{
		QDateTime created = QFileInfo(curBitstreamFile).lastModified();
		if(created > curBitstreamDateTime)
		{
			curBitstreamDateTime = created;
			loadBitstream(curBitstreamFile);
		}
	}

	if(!curFirmwareFile.isEmpty() && m_fwUpdateAutoCheckBox->isChecked())
	{
		QDateTime created = QFileInfo(curFirmwareFile).lastModified();
		if(created > curFirmwareDateTime)
		{
			curFirmwareDateTime = created;
			analyseFile(curFirmwareFile);
		}
	}

	if(!m_serial->isOpen() && serialPortAutoReconnect->isChecked())
	{
		m_console->putData("\nReconnecting...\n");
		//fillPortsInfo(); //TODO put in seperate QListArray and store as a variable. Lock the serial port display until the box is unclecked or disconnect is pressed.
	    serialPortReconnectName = serialPortInfoListBox->currentText();
		int index = serialPortInfoListBox->findText(serialPortReconnectName.toLatin1().data());
		if(index != -1) // -1 for not found
		{
			serialPortInfoListBox->setCurrentIndex(index);
			openSerialPort(true);
		}
		//qDebug() << serialPortReconnectName;
		//m_console->putData("\nReconnecting...\n");
	}

	QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();
	QStringList newSerialPorts;
	bool updated = false;
	foreach(const QSerialPortInfo &serialPortInfo, availablePorts)
	{
		if(serialPorts.indexOf(serialPortInfo.portName()) == -1)
		{
			serialPorts << serialPortInfo.portName();
			newSerialPorts << serialPortInfo.portName();
			updated = true;

			QString description = serialPortInfo.description();
			QString manufacturer = serialPortInfo.manufacturer();
			QString serialNumber = serialPortInfo.serialNumber();

			QStringList list;

			list << serialPortInfo.portName()
				 << (!description.isEmpty() ? description : blankString)
				 << (!manufacturer.isEmpty() ? manufacturer : blankString)
				 << (!serialNumber.isEmpty() ? serialNumber : blankString)
				 << serialPortInfo.systemLocation()
				 << (serialPortInfo.vendorIdentifier() ? QString::number(serialPortInfo.vendorIdentifier(), 16) : blankString)
				 << (serialPortInfo.productIdentifier() ? QString::number(serialPortInfo.productIdentifier(), 16) : blankString);

			serialPortsInfo.append(list);
		}
	}
	if(updated)
	{
		newSerialPorts.sort(Qt::CaseInsensitive);
		serialPortInfoListBox->addItems(newSerialPorts);
	}
}

void MainWindow::binaryModeConnect(){
    m_logicAnalyzerSamplesExpected=0;
    m_logicAnalyzerSamples.clear();
    if(m_serialBinaryMode->isOpen()){
        m_serialBinaryMode->close();
        showStatusMessage(tr("Disconnected"));
        m_logicAnalyzerButton->setText("Connect");
        serialPortInfoListBoxBinaryMode->setEnabled(true);

    }else{
        m_serialBinaryMode->setPortName(serialPortInfoListBoxBinaryMode->currentText());
        m_serialBinaryMode->setBaudRate(QSerialPort::Baud115200);
        m_serialBinaryMode->setDataBits(QSerialPort::Data8);
        m_serialBinaryMode->setParity(QSerialPort::NoParity);
        m_serialBinaryMode->setStopBits(QSerialPort::OneStop);
        m_serialBinaryMode->setFlowControl(QSerialPort::NoFlowControl);
        if(m_serialBinaryMode->open(QIODevice::ReadWrite))
        {
            serialPortInfoListBoxBinaryMode->setEnabled(false);
            m_logicAnalyzerButton->setText("Disconnect");
        }
        else
        {
            QMessageBox::critical(this, tr("Error!"), m_serialBinaryMode->errorString());
            m_console->putData("\nError opening binary mode port!\n");
            showStatusMessage(tr("Open error"));
        }

    }

    qDebug()<<"samples cleared " << m_logicAnalyzerSamplesExpected;

}

void MainWindow::openSerialPort(bool reconnect)
{
	//qDebug() << "Openning Serial Port" << serialPortReconnectName;
	//const SettingsDialog::Settings p = m_settings->settings();

    serialPortReconnectName = serialPortInfoListBox->currentText();
    m_serial->setPortName(serialPortInfoListBox->currentText());
    m_serial->setBaudRate(QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);
    if(m_serial->open(QIODevice::ReadWrite))
	{
		m_console->setEnabled(true);
		m_console->setLocalEchoEnabled(false);
		//m_ui->actionConnect->setEnabled(false);
		//m_ui->actionDisconnect->setEnabled(true);
		//m_ui->actionConfigure->setEnabled(false);
		//showStatusMessage(tr("Connected to %1 : %2, %3, %4, %5, %6")
						  //.arg(p.name).arg(p.stringBaudRate).arg(p.stringDataBits)
						  //.arg(p.stringParity).arg(p.stringStopBits).arg(p.stringFlowControl));
		m_console->connected();

		connectAct->setEnabled(false);
		disconnectAct->setEnabled(true);
		clearAct->setEnabled(true);

		serialPortInfoListBox->setEnabled(false);
    }
	else if(reconnect == false)
	{
		QMessageBox::critical(this, tr("Error!"), m_serial->errorString());
		m_console->putData("\nError opening port!\n");
		showStatusMessage(tr("Open error"));
    }
}

void MainWindow::closeSerialPort(bool reconnect)
{
	if(m_serial->isOpen())
		m_serial->close();

	m_console->setEnabled(false);
	m_console->disconnected();
	//m_ui->actionConnect->setEnabled(true);
	//m_ui->actionDisconnect->setEnabled(false);
	//m_ui->actionConfigure->setEnabled(true);
	showStatusMessage(tr("Disconnected"));

	connectAct->setEnabled(true);
	disconnectAct->setEnabled(false);

	serialPortInfoListBox->setEnabled(true);

	if(reconnect == false)
		serialPortAutoReconnect->setChecked(false);
}

void MainWindow::writeData(const QByteArray &data)
{
    m_serial->write(data);
}

void MainWindow::readData()
{
    const QByteArray data = m_serial->readAll();
    m_console->putData(data);
}

void MainWindow::handleError(QSerialPort::SerialPortError error)
{
	if(error == QSerialPort::ResourceError)
	{
		closeSerialPort(true);
		//QMessageBox::critical(this, tr("Critical Error"), m_serial->errorString());
		m_console->putData("\nError!\n");
	}
}


void MainWindow::binaryModeReadData(){


    m_logicAnalyzerSamples.append(m_serialBinaryMode->readAll());
    qDebug() << m_logicAnalyzerSamples.size();

    if(m_logicAnalyzerSamplesExpected==0){
        if(m_logicAnalyzerSamples.size()>=2){
            m_logicAnalyzerSamplesExpected=((quint16)(m_logicAnalyzerSamples[0]<<8)&0xFF00)|(quint16)(m_logicAnalyzerSamples[1]&0x00FF);
            m_logicAnalyzerSamples.remove(0,2);
            qDebug() << "samples:" << m_logicAnalyzerSamplesExpected;
        } else{
            return;
        }
    }




    if(m_logicAnalyzerSamples.size()>=m_logicAnalyzerSamplesExpected){
        qDebug() << "processing samples";
        //draw the samples on the plot....
        m_customPlot->clearGraphs();
        m_logicAnalyzerUpperBound=m_logicAnalyzerSamplesExpected;
        // make some data
        QVector<double> x(m_logicAnalyzerSamples.size()), y(m_logicAnalyzerSamples.size());
        quint8 maskL;
        maskL=0x01;
        for(int j = 0; j < 8; ++j)
        {
            for (int i = 0; i < m_logicAnalyzerSamples.size(); ++i)
            {
                x[i] = i;
                y[i] = ( (m_logicAnalyzerSamples[i]&maskL?0:1)+ ((double)j + ((double)j * 0.25) + 0.2) );

            }
            m_customPlot->addGraph();
            m_customPlot->graph()->setPen(QPen(QColor(165, 165, 165)));
            m_customPlot->graph()->setLineStyle((QCPGraph::lsStepLeft));
            m_customPlot->graph(j)->setData(x, y);
            //m_customPlot->graph(j)->rescaleKeyAxis();

            maskL<<=1;

        }
        m_customPlot->graph(0)->rescaleKeyAxis();
        m_customPlot->replot();
        //TODO: make rescale an option in the logic analzer panel
        //m_customPlot->xAxis->rescale(true);
        m_logicAnalyzerSamples.clear();
        m_logicAnalyzerSamplesExpected=0;

    }

}


void MainWindow::showStatusMessage(const QString &message)
{
    m_status->setText(message);
}

void MainWindow::about()
{
   QMessageBox::about(this, tr("About BusTerm"),
            tr("Terminal and tools for using and debugging the <b>Bus Pirate</b>."
               " Built on Qt5 and Qt examples. "));
}

void MainWindow::createActions()
{
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
    QToolBar *fileToolBar = addToolBar(tr("File"));

    const QIcon openIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    QAction *openAct = new QAction(openIcon, tr("&Open firmware..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open firmware"));
    connect(openAct, &QAction::triggered, this, &MainWindow::openFirmwareFile);
    fileMenu->addAction(openAct);
    fileToolBar->addAction(openAct);

    const QIcon openBitstreamIcon = QIcon::fromTheme("document-open", QIcon(":/images/open.png"));
    QAction *openBitstreamAct = new QAction(openBitstreamIcon, tr("Open &bitstream..."), this);
    openBitstreamAct->setShortcuts(QKeySequence::Open);
    openBitstreamAct->setStatusTip(tr("Open bitstream"));
    connect(openBitstreamAct, &QAction::triggered, this, &MainWindow::openBitstreamFile);
    fileMenu->addAction(openBitstreamAct);
    fileToolBar->addAction(openBitstreamAct);

    const QIcon saveIcon = QIcon::fromTheme("document-save", QIcon(":/images/save.png"));
    QAction *saveAct = new QAction(saveIcon, tr("&Save..."), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save"));
    connect(saveAct, &QAction::triggered, this, &MainWindow::save);
    fileMenu->addAction(saveAct);
    fileToolBar->addAction(saveAct);

    const QIcon printIcon = QIcon::fromTheme("document-print", QIcon(":/images/print.png"));
    QAction *printAct = new QAction(printIcon, tr("&Print..."), this);
    printAct->setShortcuts(QKeySequence::Print);
    printAct->setStatusTip(tr("Print"));
    connect(printAct, &QAction::triggered, this, &MainWindow::print);
    fileMenu->addAction(printAct);
    fileToolBar->addAction(printAct);

    fileMenu->addSeparator();

    QAction *quitAct = fileMenu->addAction(tr("&Quit"), this, &QWidget::close);
    quitAct->setShortcuts(QKeySequence::Quit);
    quitAct->setStatusTip(tr("Quit the application"));

    QMenu *connectMenu = menuBar()->addMenu(tr("&Connect"));
    QToolBar *connectToolBar = addToolBar(tr("Connect"));

	const QIcon connectIcon = QIcon::fromTheme("", QIcon(":/images/connect.png"));
	//QAction *connectAct
	connectAct = new QAction(connectIcon, tr("&Connect"), this);

	//connectAct->setShortcuts(QKeySequence::Save);
	connectAct->setStatusTip(tr("Connect to serial port"));
	connectAct->setEnabled(true);
	connect(connectAct, &QAction::triggered, this, &MainWindow::openSerialPort);
	connectMenu->addAction(connectAct);
	connectToolBar->addAction(connectAct);

	const QIcon disconnectIcon = QIcon::fromTheme("", QIcon(":/images/disconnect.png"));
	//QAction *disconnectAct
	disconnectAct = new QAction(disconnectIcon, tr("&Disconnect"), this);

	//disconnectAct->setShortcuts(QKeySequence::Print);
	disconnectAct->setStatusTip(tr("Disconnect serial port"));
	disconnectAct->setEnabled(false);
	connect(disconnectAct, &QAction::triggered, this, &MainWindow::closeSerialPort);
	connectMenu->addAction(disconnectAct);
	connectToolBar->addAction(disconnectAct);

	const QIcon clearIcon = QIcon::fromTheme("", QIcon(":/images/clear.png"));
	//QAction *clearAct
	clearAct = new QAction(clearIcon, tr("&Clear"), this);

	//clearAct->setShortcuts(QKeySequence::Print);
	clearAct->setStatusTip(tr("Clear terminal"));
	clearAct->setEnabled(false);
	connect(clearAct, &QAction::triggered, m_console, &Console::clear);
	connectMenu->addAction(clearAct);
	connectToolBar->addAction(clearAct);

	viewMenu = menuBar()->addMenu(tr("&View"));

	menuBar()->addSeparator();

	QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

	QAction *aboutAct = helpMenu->addAction(tr("&About"), this, &MainWindow::about);
	aboutAct->setStatusTip(tr("Show the application's About box"));

	QAction *aboutQtAct = helpMenu->addAction(tr("About &Qt"), qApp, &QApplication::aboutQt);
	aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
}

void MainWindow::createStatusBar()
{
    statusBar()->showMessage(tr("Ready"));
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
  if (event->type() == QEvent::Resize && obj == laDock) {
      QResizeEvent *resizeEvent = static_cast<QResizeEvent*>(event);
      qDebug("Dock Resized (New Size) - Width: %d Height: %d",
             resizeEvent->size().width(),
             resizeEvent->size().height());
      m_customPlot->setGeometry(QRect(0, 0, resizeEvent->size().width(), 252)); //resizeEvent->size().height()
  }
  return QWidget::eventFilter(obj, event);
}

void MainWindow::createDockWindows()
{
	QDockWidget *dock;
	QFormLayout *flayout;
	QWidget *multiWidget;

	//create the logic analyzer plot
	QWidget *plot_frame_temp = new QWidget();
	//plot_frame_temp->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
	m_customPlot = new QCustomPlot(plot_frame_temp);
	m_customPlot->axisRect()->setupFullAxesBox(true);
	m_customPlot->setBackground(Qt::black);
	// zoom and drag only on horrizontal axis
	m_customPlot->axisRect()->setRangeZoomAxes(m_customPlot->xAxis, nullptr);
	m_customPlot->axisRect()->setRangeDragAxes(m_customPlot->xAxis, nullptr);
	m_customPlot->axisRect()->setAutoMargins(QCP::msTop);
	//setup the top axis and labels
    //QSharedPointer<QCPAxisTickerTime> timeTicker(new QCPAxisTickerTime);
    //m_customPlot->xAxis2->setTicker(timeTicker);
    //timeTicker->setTimeFormat("%zms");
	m_customPlot->yAxis->scaleRange(2, 0);
	m_customPlot->xAxis->setVisible(false);
	m_customPlot->xAxis2->setVisible(true);
	m_customPlot->xAxis2->setTicks(true);
	m_customPlot->xAxis2->setTickLabels(true);
	m_customPlot->xAxis2->setTickPen(QColor(136, 136, 136));
	m_customPlot->xAxis2->setTickLength(0, 5);
	m_customPlot->xAxis2->setTickLabelColor(QColor(136, 136, 136));
	m_customPlot->xAxis2->setSubTickPen(Qt::NoPen);
	m_customPlot->xAxis2->setBasePen(Qt::NoPen);
	QFont font;
	font.setStyleStrategy(QFont::PreferOutline);
	m_customPlot->xAxis2->setTickLabelFont(font);
	//setup the left axis and hide
	m_customPlot->yAxis->setVisible(false);
	m_customPlot->yAxis->setRangeReversed(true);
	m_customPlot->yAxis2->setVisible(false);
	// make some data
    //use hello world in ASCII with decoder for startup :)
    QVector<double> x(500), y(500);
	for(int j = 0; j < 8; ++j)
	{
		for (int i = 0; i < 500; ++i)
		{
			x[i] = i;
			y[i] = (rand() % 2 + ((double)j + ((double)j * 0.25) + 0.2) );
		}
		m_customPlot->addGraph();
		m_customPlot->graph()->setPen(QPen(QColor(165, 165, 165)));
		m_customPlot->graph()->setLineStyle((QCPGraph::lsStepLeft));
		m_customPlot->graph(j)->setData(x, y);
	}
    m_logicAnalyzerUpperBound=500;

	//add data to graph

	// set some options
	m_customPlot->setNotAntialiasedElements(QCP::aeAll);
	m_customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

	//m_customPlot->setGeometry(QRect(0, 171, 390, 252));
	//m_customPlot->axisRect()->setMinimumSize(500,400);
	//m_customPlot->setMinimumSize(QSize(500,500));
	//m_customPlot->addLayer("image", m_customPlot->layer("main"), QCustomPlot::LayerInsertMode::limBelow);

	QBoxLayout *laLayout = new  QHBoxLayout();
	laLayout->setSpacing(0);
	laLayout->setContentsMargins(0, 0, 0, 0);
	QWidget *frame = new QWidget(this);
	//frame->setGeometry(0, 0, 171, 43);
	frame->setStyleSheet("background-image: url(:/images/la-8chan.png)");
	plot_frame_temp->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
	frame->setMinimumSize(171, 252);
	frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

	laLayout->addWidget(frame, 0, (Qt::AlignLeft | Qt::AlignTop));
	laLayout->addWidget(plot_frame_temp);

	QWidget *laWidget = new QWidget();
	laWidget->setLayout(laLayout);
	//add plot to dock
	laDock = new QDockWidget(tr("Logic Analyzer"), this);
	laDock->installEventFilter(this);
	//laDock->setMinimumSize(m_customPlot->minimumSize());
	laDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	laDock->setWidget(laWidget);
	addDockWidget(Qt::BottomDockWidgetArea, laDock);
	viewMenu->addAction(laDock->toggleViewAction());

	// DEC/HEX/BIN/ASCII converter
	flayout = new QFormLayout;
	conv_dec = new QLineEdit();
	flayout->addRow(new QLabel(tr("DEC")), conv_dec);
	conv_hex = new QLineEdit();
	flayout->addRow(new QLabel(tr("HEX 0x")), conv_hex);
	conv_bin = new QLineEdit();
	flayout->addRow(new QLabel(tr("BIN 0b")), conv_bin);
	conv_ascii = new QLineEdit();
	flayout->addRow(new QLabel(tr("ASCII")), conv_ascii);
	//new dock
	dock = new QDockWidget(tr("Convert Values"), this);
	dock->setMinimumWidth(200);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	//add layout to a new widget
	multiWidget = new QWidget();
	multiWidget->setLayout(flayout);
	//add to dock
	dock->setWidget(multiWidget);
	addDockWidget(Qt::RightDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());
	//binary calculator place holder
	//QList<QStringList> portList=m_settings->fillPortsInfo();

	flayout = new QFormLayout;

	serialPortInfoListBox = new QComboBox();
	flayout->addRow(serialPortInfoListBox);
	serialPortDescriptionLabel= new QLabel(tr("Description"));
	flayout->addRow(serialPortDescriptionLabel);
	serialPortManufacturerLabel = new QLabel(tr("Manufacturer"));
	flayout->addRow(serialPortManufacturerLabel);
	serialPortSerialNumberLabel = new QLabel(tr("Serial number:"));
	flayout->addRow(serialPortSerialNumberLabel);
	serialPortLocationLabel = new QLabel(tr("Location:"));
	flayout->addRow(serialPortLocationLabel);
	serialPortVidLabel = new QLabel(tr("Vendor ID:"));
	flayout->addRow(serialPortVidLabel);
	serialPortPidLabel = new QLabel(tr("Product ID:"));
	flayout->addRow(serialPortPidLabel);
	serialPortAutoReconnect = new QCheckBox(tr("Reconnect"), this);
	flayout->addRow(serialPortAutoReconnect);

	//new dock
	dock = new QDockWidget(tr("Serial Port"), this);
	dock->setMinimumWidth(200);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	//add layout to a new widget
	multiWidget = new QWidget();
	multiWidget->setLayout(flayout);
	//add to dock
	dock->setWidget(multiWidget);
	addDockWidget(Qt::RightDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());








    flayout = new QFormLayout;

    serialPortInfoListBoxBinaryMode = new QComboBox();
    flayout->addRow(serialPortInfoListBoxBinaryMode);

    m_logicAnalyzerButton = new QPushButton("&Connect", this);
    connect(m_logicAnalyzerButton, &QPushButton::released, this, &MainWindow::binaryModeConnect);
    flayout->addRow(m_logicAnalyzerButton);
    //new dock
    dock = new QDockWidget(tr("Binmode Serial Port"), this);
    dock->setMinimumWidth(200);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    //add layout to a new widget
    multiWidget = new QWidget();
    multiWidget->setLayout(flayout);
    //add to dock
    dock->setWidget(multiWidget);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());










	dock = new QDockWidget(tr("Binary Calculator"), this);
	dock->setMinimumWidth(200);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	QWidget* binCalc = new QWidget();
	dock->setWidget(binCalc);
	addDockWidget(Qt::RightDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());

	//Firmware update dock
	flayout = new QFormLayout;
	m_fwUpdateFileLabel = new QLabel(tr("None..."));
	flayout->addRow(m_fwUpdateFileLabel);
	QPushButton * fwSelectFile = new QPushButton("Select", this);
	connect(fwSelectFile, &QPushButton::released, this, &MainWindow::openFirmwareFile);
	flayout->addRow(fwSelectFile);
	m_fwUpdateProgressBar = new QProgressBar;
	//fwUpdateProgressBar->setMaximumWidth(1000);
	m_fwUpdateProgressBar->setRange(0,100);
	flayout->addRow(m_fwUpdateProgressBar);
	m_fwUpdateAutoCheckBox = new QCheckBox(tr("Auto update"), this);
	m_fwUpdateAutoCheckBox->setCheckState(Qt::Checked);
	flayout->addRow(m_fwUpdateAutoCheckBox);

	//add to dock...
	dock = new QDockWidget(tr("Firmware Update"), this);
	dock->setMinimumWidth(200);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	multiWidget = new QWidget();
	multiWidget->setLayout(flayout);
	dock->setWidget(multiWidget);
	addDockWidget(Qt::RightDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());

	//bitstream update dock
	flayout = new QFormLayout;
	m_bsUpdateFileLabel = new QLabel(tr("None..."));
	flayout->addRow(m_bsUpdateFileLabel);
	QPushButton * bsSelectFile = new QPushButton("Select", this);
	connect(bsSelectFile, &QPushButton::released, this, &MainWindow::openBitstreamFile);
	flayout->addRow(bsSelectFile);
	m_bsUpdateProgressBar = new QProgressBar;
	//bsUpdateProgressBar->setMaximumWidth(1000);
	m_bsUpdateProgressBar->setRange(0, 100);
	flayout->addRow(m_bsUpdateProgressBar);
	m_bsUpdateAutoCheckBox = new QCheckBox(tr("Auto update"), this);
	m_bsUpdateAutoCheckBox->setCheckState(Qt::Checked);
	flayout->addRow(m_bsUpdateAutoCheckBox);
	//add to dock...

	dock = new QDockWidget(tr("Bitstream Update"), this);
	dock->setMinimumWidth(200);
	dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
	multiWidget = new QWidget();
	multiWidget->setLayout(flayout);
	dock->setWidget(multiWidget);
	addDockWidget(Qt::RightDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());
}

/*
void MainWindow::serialPortChanged(const QString &text)
{
	qDebug() << text;
}
*/

void MainWindow::fillPortsInfo()
{
	//qDebug() << "Fill ports info";

	QList<QSerialPortInfo> availablePorts = QSerialPortInfo::availablePorts();

	foreach(const QSerialPortInfo &serialPortInfo, availablePorts)
	{
		serialPorts << serialPortInfo.portName();

		QString description = serialPortInfo.description();
		QString manufacturer = serialPortInfo.manufacturer();
		QString serialNumber = serialPortInfo.serialNumber();

		QStringList list;

		list << serialPortInfo.portName()
			 << (!description.isEmpty() ? description : blankString)
			 << (!manufacturer.isEmpty() ? manufacturer : blankString)
			 << (!serialNumber.isEmpty() ? serialNumber : blankString)
			 << serialPortInfo.systemLocation()
			 << (serialPortInfo.vendorIdentifier() ? QString::number(serialPortInfo.vendorIdentifier(), 16) : blankString)
			 << (serialPortInfo.productIdentifier() ? QString::number(serialPortInfo.productIdentifier(), 16) : blankString);

		serialPortsInfo.append(list);

		//qDebug() << "Connected_1" << serialPortInfo.portName();
	}

	serialPortInfoListBox->clear();
	serialPorts.sort(Qt::CaseInsensitive);
	serialPortInfoListBox->addItems(serialPorts);
	selectedPort = serialPorts[0];
	serialPortReconnectName = selectedPort;

    serialPortInfoListBoxBinaryMode->addItems(serialPorts);

	/*
	serialPortInfoListBox->clear();

	QString description;
	QString manufacturer;
	QString serialNumber;

	//const auto infos = QSerialPortInfo::availablePorts();
	//QList<QSerialPortInfo> infos = QSerialPortInfo::availablePorts();

	for(const QSerialPortInfo &info : infos)
	{
		QStringList list;

		description = info.description();
		manufacturer = info.manufacturer();
		serialNumber = info.serialNumber();
		list << info.portName()
			 << (!description.isEmpty() ? description : blankString)
			 << (!manufacturer.isEmpty() ? manufacturer : blankString)
			 << (!serialNumber.isEmpty() ? serialNumber : blankString)
			 << info.systemLocation()
			 << (info.vendorIdentifier() ? QString::number(info.vendorIdentifier(), 16) : blankString)
			 << (info.productIdentifier() ? QString::number(info.productIdentifier(), 16) : blankString);

		serialPortInfoListBox->addItem(list.first(), list);
	}

	//qDebug() << serialPortInfoListBox->count();
	*/
}

void MainWindow::showPortInfo(int idx)
{
    if(idx == -1 || serialPortInfoListBox->count() == 0)
	{
        return;
	}

	QString newSelectedPort = serialPortInfoListBox->currentText();
	if(newSelectedPort.compare(selectedPort) != 0)
	{
		selectedPort = newSelectedPort;
		serialPortReconnectName = selectedPort;
	}

	int x = serialPortsInfo.count();
	for(int i = 0; i < x; i++)
	{
		//qDebug() << serialPortsInfo[i].first();
		if(serialPortsInfo[i].first().compare(selectedPort) == 0)
		{
			//qDebug() << serialPortsInfo[i].first();

			const QStringList list = serialPortsInfo[i];
			serialPortDescriptionLabel->setText(tr("Description: %1").arg(list.count() > 1 ? list.at(1) : tr(blankString)));
			serialPortManufacturerLabel->setText(tr("Manufacturer: %1").arg(list.count() > 2 ? list.at(2) : tr(blankString)));
			serialPortSerialNumberLabel->setText(tr("Serial number: %1").arg(list.count() > 3 ? list.at(3) : tr(blankString)));
			serialPortLocationLabel->setText(tr("Location: %1").arg(list.count() > 4 ? list.at(4) : tr(blankString)));
			serialPortVidLabel->setText(tr("Vendor ID: %1").arg(list.count() > 5 ? list.at(5) : tr(blankString)));
			serialPortPidLabel->setText(tr("Product ID: %1").arg(list.count() > 6 ? list.at(6) : tr(blankString)));
			break;
		}
	}

	/*const QStringList list = serialPortInfoListBox->itemData(idx).toStringList();
    serialPortDescriptionLabel->setText(tr("Description: %1").arg(list.count() > 1 ? list.at(1) : tr(blankString)));
    serialPortManufacturerLabel->setText(tr("Manufacturer: %1").arg(list.count() > 2 ? list.at(2) : tr(blankString)));
    serialPortSerialNumberLabel->setText(tr("Serial number: %1").arg(list.count() > 3 ? list.at(3) : tr(blankString)));
    serialPortLocationLabel->setText(tr("Location: %1").arg(list.count() > 4 ? list.at(4) : tr(blankString)));
    serialPortVidLabel->setText(tr("Vendor ID: %1").arg(list.count() > 5 ? list.at(5) : tr(blankString)));
    serialPortPidLabel->setText(tr("Product ID: %1").arg(list.count() > 6 ? list.at(6) : tr(blankString)));*/
}

//TODO: add a plot of the number waveform of the value at the bottom
void MainWindow::convert(const QString &text)
{
    char base;
    QObject* obj = sender();

    if(obj == conv_dec)
        base = 10;
	else if(obj == conv_hex)
        base = 16;
	else if(obj == conv_bin)
        base = 2;
	else
        base = 10;

    long long num;
    if(obj == conv_ascii)
	{
        QChar c;
        if(text.length() > 0)
            c = text.at(0);
        num = c.toLatin1();
	}
	else
	{
		bool ok;
		num = text.toLongLong(&ok, base);
	}

	QString hex = QString::number(num, 16).toUpper();
	QString dec = QString::number(num, 10);
	QString octal = QString::number(num, 8);
	QString bin = QString::number(num, 2);
	QString ascii;
	QChar a = uint8_t(num);
	if(a >= 32 && a <= 127)
		ascii = a;
	conv_dec->setText(dec);
	conv_hex->setText(hex);
	conv_bin->setText(bin);
	conv_ascii->setText(ascii);
}

void MainWindow::openFirmwareFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Open Bus Pirate firmware", curPath, "HEX Files (*.hex)");
	if(!fileName.isEmpty())
	{
		m_fwUpdateFileLabel->setText(QFileInfo(fileName).fileName().left(30));
		curFirmwareFile = fileName;
		curFirmwareDateTime = QFileInfo(fileName).lastModified();
		analyseFile(fileName);
	}
}

void MainWindow::analyseFile(const QString &fileName)
{
    QThread* thread = new QThread;
    loadhexworker* worker = new loadhexworker(fileName);
    worker->moveToThread(thread);
    connect(worker, &loadhexworker::info, this, &MainWindow::consoleLog);
    connect(worker, &loadhexworker::error, this, &MainWindow::consoleLog);
    //connect(worker, &loadhexworker::timeout, this, &MainWindow::consoleLog);
    //connect(worker, &loadhexworker::progress, this, &MainWindow::progressBar);
    connect(thread, &QThread::started, worker, &loadhexworker::load);
    connect(worker, &loadhexworker::output, this, &MainWindow::processBootload);
    connect(worker, &loadhexworker::finished, thread, &QThread::quit);
    connect(worker, &loadhexworker::finished, worker, &loadhexworker::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    qDebug()<<"From main thread: "<<QThread::currentThreadId();
    thread->start();
}

void MainWindow::processBootload(const QByteArray &firmware)
{
    QThread* thread = new QThread;
    loaderds30loader* worker = new loaderds30loader(serialPortInfoListBox->currentText(),firmware);
    worker->moveToThread(thread);
    connect(worker, &loaderds30loader::info, this, &MainWindow::consoleLog);
    connect(worker, &loaderds30loader::error, this, &MainWindow::consoleLog);
    connect(worker, &loaderds30loader::timeout, this, &MainWindow::consoleLog);
    connect(worker, &loaderds30loader::progress, this, &MainWindow::fwUpdateProgress);
    connect(thread, &QThread::started, worker, &loaderds30loader::load);
    connect(worker, &loaderds30loader::finished, thread, &QThread::quit);
    connect(worker, &loaderds30loader::finished, worker, &loaderds30loader::deleteLater);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    qDebug()<<"From main thread: "<<QThread::currentThreadId();
    thread->start();
}

void MainWindow::fwUpdateProgress(const quint8 &c)
{
    m_fwUpdateProgressBar->setValue(c);
}

void MainWindow::openBitstreamFile()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Open Bus Pirate bitstream", curPath,	"BIN Files (*.bin)");
	if(!fileName.isEmpty())
	{
		m_bsUpdateFileLabel->setText(QFileInfo(fileName).fileName().left(30));
		curBitstreamFile = fileName;
		curBitstreamDateTime = QFileInfo(fileName).lastModified();
		loadBitstream(fileName);
	}
}

void MainWindow::loadBitstream(const QString &fileName)
{
    if(m_bsComplete==true){
        QThread* thread = new QThread;
        loadIce40* worker = new loadIce40(serialPortInfoListBoxBinaryMode->currentText(),fileName);
        worker->moveToThread(thread);
        connect(worker, &loadIce40::info, this, &MainWindow::consoleLog);
        connect(worker, &loadIce40::error, this, &MainWindow::consoleLog);
        //connect(worker, &loadhexworker::timeout, this, &MainWindow::consoleLog);
        connect(worker, &loadIce40::progress, this, &MainWindow::bsUpdateProgress);
        connect(thread, &QThread::started, worker, &loadIce40::load);
        //connect(worker, &loadhexworker::output, this, &MainWindow::processBootload);
        connect(worker, &loadIce40::finished,this, &MainWindow::bsComplete);
        connect(worker, &loadIce40::finished, thread, &QThread::quit);
        connect(worker, &loadIce40::finished, worker, &loadIce40::deleteLater);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        qDebug() << "From main thread: " << QThread::currentThreadId();
        m_bsComplete=false;
        thread->start();
    }
}
void MainWindow::bsComplete(){
    m_bsComplete=true;
}

void MainWindow::bsUpdateProgress(const quint8 &c)
{
	m_bsUpdateProgressBar->setValue(c);
}

void MainWindow::consoleLog(const QString &s)
{
	m_console->putData(s.toLatin1());
}

void MainWindow::processError(const QString &s)
{
	m_console->putData(s.toLatin1());
	/*
	setControlsEnabled(true);
	m_statusLabel->setText(tr("Status: Not running, %1.").arg(s));
	m_trafficLabel->setText(tr("No traffic."));
	*/
}

void MainWindow::processTimeout(const QString &s)
{
	m_console->putData(s.toLatin1());
	/*
	setControlsEnabled(true);
	m_statusLabel->setText(tr("Status: Running, %1.").arg(s));
	m_trafficLabel->setText(tr("No traffic."));
	*/
}

/*
//TODO: lock this to full range too...
void MainWindow::horzScrollBarChanged(int value)
{
	if(qAbs(ui->plot->xAxis->range().center()-value/100.0) > 0.01) // if user is dragging plot, we don't want to replot twice
	{
		ui->plot->xAxis->setRange(value/100.0, ui->plot->xAxis->range().size(), Qt::AlignCenter);
		ui->plot->replot();
	}
}
*/

// lock the axis: https://www.qcustomplot.com/index.php/support/forum/289
void MainWindow::lockRange(QCPRange range)
{
	//ui->horizontalScrollBar->setValue(qRound(range.center()*100.0)); // adjust position of scroll bar slider
	//ui->horizontalScrollBar->setPageStep(qRound(range.size()*100.0)); // adjust size of scroll bar slider

	double lowerBound = 0;
    double upperBound = (double)m_logicAnalyzerUpperBound; // note: code assumes lowerBound < upperBound

    qDebug() << "stored ubound:" << m_logicAnalyzerUpperBound;
    qDebug() << "ubound:" << upperBound;

	QCPRange fixedRange(range);
    qDebug() << "range.upper:" << fixedRange.upper;
	if(fixedRange.lower < lowerBound)
	{
		fixedRange.lower = lowerBound;
		fixedRange.upper = lowerBound + range.size();
        if(fixedRange.upper > upperBound || qFuzzyCompare(range.size(), upperBound-lowerBound))
            fixedRange.upper = upperBound;
        //m_customPlot->xAxis->setRange(fixedRange); // adapt this line to use your plot/axis
	}
    else if(fixedRange.upper > upperBound)
	{
        fixedRange.upper = upperBound;
        fixedRange.lower = upperBound - range.size();
        if(fixedRange.lower < lowerBound || qFuzzyCompare(range.size(), upperBound-lowerBound))
			fixedRange.lower = lowerBound;
        //m_customPlot->xAxis->setRange(fixedRange); // adapt this line to use your plot/axis
	}
}

void MainWindow::print()
{
	/*
	#if QT_CONFIG(printdialog)
		QTextDocument *document = textEdit->document();
		QPrinter printer;
		QPrintDialog dlg(&printer, this);
		if(dlg.exec() != QDialog::Accepted)
		{
			return;
		}
		document->print(&printer);
		statusBar()->showMessage(tr("Ready"), 2000);
	#endif
	*/
}

void MainWindow::save()
{
	/*
	QMimeDatabase mimeDatabase;
	QString fileName = QFileDialog::getSaveFileName(this,
                        tr("Choose a file name"), ".",
                        mimeDatabase.mimeTypeForName("text/html").filterString());
	if(fileName.isEmpty())
		return;
	QFile file(fileName);
	if(!file.open(QFile::WriteOnly | QFile::Text))
	{
		QMessageBox::warning(this, tr("Dock Widgets"), tr("Cannot write file %1:\n%2.").arg(QDir::toNativeSeparators(fileName), file.errorString()));
		return;
	}

	QTextStream out(&file);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	out << textEdit->toHtml();
	QApplication::restoreOverrideCursor();

	statusBar()->showMessage(tr("Saved '%1'").arg(fileName), 2000);
	*/
}
