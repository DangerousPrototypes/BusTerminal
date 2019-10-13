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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSerialPort>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
class QLabel;
//class QDeviceWatcher;
QT_END_NAMESPACE

class Console;
class SettingsDialog;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow();

private slots:
	void save();
	void print();
	void about();
	void openSerialPort(bool reconnect);
	void closeSerialPort(bool reconnect);
	void writeData(const QByteArray &data);
	void readData();
	void showPortInfo(int idx);
	void handleError(QSerialPort::SerialPortError error);
	void convert(const QString &text);
	//void horzScrollBarChanged(int value);
	void lockRange(QCPRange range);
	void fillPortsInfo();
	//void serialPortChanged(const QString &text);
	//void transaction();
	void consoleLog(const QString &s);
	void processError(const QString &s);
	void processTimeout(const QString &s);
	void processBootload(const QByteArray &firmware);
	void openBitstreamFile();
	void openFirmwareFile();
    void openImageFile();
    void openFontFile();
	void fwUpdateProgress(const quint8 &c);
	void bsUpdateProgress(const quint8 &c);
    void imageUpdateProgress(const quint8 &c);
    void bsComplete();
	void periodicEvents();
    void binaryModeReadData();
    void binaryModeConnect();

protected:
  bool eventFilter(QObject *obj, QEvent *event);

private:
	void createActions();
	void createStatusBar();
	void createDockWindows();
	void showStatusMessage(const QString &message);
	void analyseFile(const QString &fileName);
	void loadBitstream(const QString &fileName);
    void loadImage(const QString &fileName);

	QDockWidget *laDock;

	//serial port stuff
	struct Settings {
		QString name;
		qint32 baudRate;
		QString stringBaudRate;
		QSerialPort::DataBits dataBits;
		QString stringDataBits;
		QSerialPort::Parity parity;
		QString stringParity;
		QSerialPort::StopBits stopBits;
		QString stringStopBits;
		QSerialPort::FlowControl flowControl;
		QString stringFlowControl;
		bool localEchoEnabled;
	};
	QLabel *m_status = nullptr;
	Console *m_console = nullptr;
	//SettingsDialog *m_settings = nullptr;
	QSerialPort *m_serial = nullptr;
	//QDeviceWatcher *serialPortWatcher;

	QMenu *viewMenu;
	QCustomPlot *m_customPlot;
	QLineEdit *echoLineEdit;
	QLineEdit *conv_dec;
	QLineEdit *conv_hex;
	QLineEdit *conv_bin;
	QLineEdit *conv_ascii;

	QComboBox *serialPortInfoListBox;
	QLabel *serialPortDescriptionLabel;
	QLabel *serialPortLocationLabel;
	QLabel *serialPortManufacturerLabel;
	QLabel *serialPortPidLabel;
	QLabel *serialPortSerialNumberLabel;
	QLabel *serialPortVidLabel;
	QCheckBox *serialPortAutoReconnect;
	QString serialPortReconnectName;

    QSerialPort *m_serialBinaryMode = nullptr;
    QComboBox *serialPortInfoListBoxBinaryMode;
    QByteArray m_logicAnalyzerSamples;
    quint16 m_logicAnalyzerSamplesExpected;
    double m_logicAnalyzerUpperBound;
    QPushButton *m_logicAnalyzerButton;

	//always running UI update timer for use in auto connect and auto updates
	QTimer *periodicTimer;
	// firmware update dock
	QProgressBar *m_fwUpdateProgressBar;
	QLabel * m_fwUpdateFileLabel;
	QCheckBox *m_fwUpdateAutoCheckBox;
	QString curFirmwareFile;
	QDateTime curFirmwareDateTime;
	//bitstream update dock
	QProgressBar *m_bsUpdateProgressBar;
	QLabel * m_bsUpdateFileLabel;
	QCheckBox *m_bsUpdateAutoCheckBox;
	QString curBitstreamFile;
	QDateTime curBitstreamDateTime;
    //image loader
    QProgressBar *m_imageUpdateProgressBar;
    QLabel * m_imageUpdateFileLabel;
    QString curImageFile;
    QLabel * m_imageLabel;

    bool m_bsComplete;

	QString curFile;
	QString curPath;

	QAction *connectAct;
	QAction *disconnectAct;
	QAction *clearAct;

	QVector<QStringList> serialPortsInfo;
	QStringList serialPorts;
	QString selectedPort;
};
#endif
