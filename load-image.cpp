#ifndef LOAD_IMAGE
#define LOAD_IMAGE

#define BOOTLOADER_HELLO_STR "\xC0"
#define BOOTLOADER_OK 0x4B

#include <QtSerialPort>
#include <QThread>
#include <QImage>

#include "load-image.h"

loadImageWorker::loadImageWorker(const QString &portName, const QString &fileName)
{
    m_portName = portName;
    m_filename=fileName;
}

loadImageWorker::~loadImageWorker()
{
}

void loadImageWorker::load()
{
    qDebug() <<"Worker::loadImage called from: " << QThread::currentThreadId();

    QImage img;
    img.load(m_filename);
    qDebug() << img.width() << img.height();
    img=img.convertToFormat(QImage::Format_RGB16);
    QByteArray blob = QByteArray::fromRawData(reinterpret_cast<const char*>(img.constBits()), img.sizeInBytes());
    //QByteArray blob;
    //uchar *bits = img.bits();

    qDebug() << img.width() << img.height() << blob.size();

    /*for (int i = 0; i < (img.width() * img.height() * 3); i++)
    {
        blob.append(reinterpret_cast<const char*>(bits[i]));
    }*/


    //QString outputError=tr("\nBitstream %1 is %2 bytes.\n").arg(m_filename).arg(file.size());
    //emit info(outputError);

    m_serial = new QSerialPort;

    if (m_portName.isEmpty()) {
        emit error(tr("No port name specified"));
        return;
    }

    m_serial->close();
    m_serial->setPortName(m_portName);
    m_serial->setBaudRate(921600);//QSerialPort::Baud115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (!m_serial->open(QIODevice::ReadWrite)) {
        emit error(tr("Can't open %1, error code %2")
                   .arg(m_portName).arg(m_serial->error()));
        return;
    }

    emit info(tr("Sending Hello to the Bootloader...\n"));

    //send HELLO
    QByteArray currentRequest;
    currentRequest.append(BOOTLOADER_HELLO_STR);
    QByteArray responseData=sendCommandAndWaitForResponse(currentRequest,10);

    //qDebug() << responseData;

    if( responseData.size() != 1 || responseData.at(0) != BOOTLOADER_OK ) {
        emit error(tr("Bootloader did not reply."));
    }

  /*  if( (unsigned char)responseData.at(0) != PIC_ID ) {
        emit error(tr("Wrong chip ID."));
    }
*/
    emit info(tr("OK\n"));

    //send the firmware
    qint32 res=sendFirmware(blob);

   if( res == 0 ) {
       emit info("\nImage updated successfully :)!\n\n");
       emit info("\nUpdating display...");
       currentRequest.clear();
       currentRequest.append(0x05);
       responseData=sendCommandAndWaitForResponse(currentRequest,10000);

       if(responseData.size()==0 || responseData.at(0)!=BOOTLOADER_OK){
             emit info("failed!\n");
       }
       emit info("ok\n");
   } else {
       emit info("\nError updating image :(\n\n");
   }


   m_serial->close();

   emit finished();

}

qint32 loadImageWorker::sendFirmware(QByteArray blob){

    qint32 u_addr=0;
    quint32 page  = 0;
    qint32 done  = 0;
    quint32 row   = 0;

    QByteArray command;

    //add blob size to first three bytes MSB
    u_addr=blob.size();
    //blob.prepend((char) ((u_addr & 0x00FF0000) >> 16 ));
    //blob.prepend((char) ((u_addr & 0x0000FF00) >> 8 ));
    //blob.prepend((char) ((u_addr & 0x000000FF) >> 0 ));
    u_addr=0;

        //write 8 rows
        for( page = 0; u_addr<blob.size(); page ++, u_addr += 256)
        {
            command.clear();
            command.append( 0x04); //write command
            command.append( (char) ((u_addr & 0x00FF0000) >> 16 ));
            command.append( (char) ((u_addr & 0x0000FF00) >>  8 ));
            command.append( (char) ((u_addr & 0x000000FF) >>  0));
            //command.append( char(PIC_ROW_SIZE + 0x01)); //DATA_LENGTH + CRC
            command.append(blob.mid(u_addr,256));
            if(u_addr+256>blob.size()){
                command.append((u_addr+256)-blob.size(), (char)0xff);
                qDebug()<< command.size();
            }
            //command.append( (char) makeCrc(command));

            emit info(QString("Writing page %1, %3...").arg(page).arg(u_addr,1,16).toLatin1());


           //qDebug() << command.toHex(',');

            QByteArray responseData=sendCommandAndWaitForResponse(command,10);
            //qDebug()<<responseData;

            if(responseData.size()==0 || responseData.at(0)!=BOOTLOADER_OK){
                  emit info("failed!\n");
                  return -1;
            }
            emit info("ok\n");

            //done += PIC_ROW_SIZE;
            emit progress((quint8)(((u_addr+2)*100)/blob.size()));
            //qDebug()<< (quint8)(((u_addr+2)*100)/blob.size());
        }

        emit progress((quint8)100);

    return 0;

}


QByteArray loadImageWorker::sendCommandAndWaitForResponse(QByteArray currentRequest, quint16 delay){
    QByteArray responseData;
    m_serial->write(currentRequest);
    if (m_serial->waitForBytesWritten(delay*10)) {
        // read response
        if (m_serial->waitForReadyRead(delay*10)) {
            responseData = m_serial->readAll();
            while (m_serial->waitForReadyRead(delay))
                responseData += m_serial->readAll();
        } else {
            emit info(tr("Wait read response timeout %1")
                         .arg(QTime::currentTime().toString()));
        }
    } else {
        emit info(tr("Wait write request timeout %1")
                     .arg(QTime::currentTime().toString()));
    }
    return responseData;
}


#endif
