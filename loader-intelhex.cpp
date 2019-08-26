#include "loader-intelhex.h"
#include <QtDebug>
//imported from qtIntegrity, dont know which we really need
#include<QTime>
#include<QDate>
#include<QDateTime>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include <iomanip>
#include <QThread>
// !
#include "intelhexclass.h"

loadhexworker::loadhexworker(const QString &fileName)
{
    m_filename=fileName;
}

loadhexworker::~loadhexworker()
{
}

void loadhexworker::load()
{
    qDebug() <<"Worker::loadhexworker called from: " << QThread::currentThreadId();

    intelhex ihFile;
    std::ifstream intelHexInput;
    QTime t;
    int elapsedTime;

    intelHexInput.open(m_filename.toLatin1(), ifstream::in);

    if(!intelHexInput.good())
    {
        QString outputError=tr("Flash firmware error. Cannot read file %1.").arg(m_filename);
        emit error(outputError);

    }else{

        t.start();

        intelHexInput >> ihFile;

        elapsedTime = t.elapsed();

        if (ihFile.getNoWarnings() < 1 && ihFile.getNoErrors() < 1)
        {
            QString outputMessage;

            outputMessage += tr("No issues found in file %1.") .arg(m_filename);
            outputMessage += tr("\nDecode took %1ms.\n") .arg(elapsedTime);

            QDateTime dateTime = QDateTime::currentDateTime();
            QString dateTimeString = dateTime.toString();
            outputMessage += tr("Date and time: ") + dateTimeString + "\n";

            emit info(outputMessage);

            QByteArray firmware;
            unsigned char f=0xff;

            //I think there is a stream operator to get the HEX out but I don't understand how to use it yet... lame
            ihFile.begin();
            while(ihFile.endOfData()==false){
                 ihFile.getData(&f);
                 firmware.append((char)f);
                 ++ihFile;
            }

            emit output(firmware);
        }
        else
        {
            QString outputMessage;

            //statusBar()->showMessage(tr("Issues found in file"));
            outputMessage = tr("File %1 contains the following issues:\n").arg(m_filename);

            if (ihFile.getNoWarnings() > 0)
            {
                outputMessage += "\n";

                while(ihFile.getNoWarnings() > 0)
                {
                    QString qmessage;
                    string message;

                    ihFile.popNextWarning(message);

                    qmessage = QString::fromStdString(message);

                    outputMessage += tr("WARNING: ");
                    outputMessage += qmessage;
                    outputMessage += "\n";
                }
            }
            if (ihFile.getNoErrors() > 0)
            {
                outputMessage += "\n";

                while (ihFile.getNoErrors() > 0)
                {
                    QString qmessage;
                    string message;

                    ihFile.popNextError(message);

                    qmessage = QString::fromStdString(message);

                    outputMessage += tr("ERROR: ") + qmessage + "\n";
                }
            }

            outputMessage += tr("\nDecode took %1ms.\n") .arg(elapsedTime);

            QDateTime dateTime = QDateTime::currentDateTime();
            QString dateTimeString = dateTime.toString();
            outputMessage += tr("Date and time: ") + dateTimeString + "\n";

            emit info(outputMessage);
        }
    }

    emit finished();

}
