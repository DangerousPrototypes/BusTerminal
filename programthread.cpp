/****************************************************************************
**
** Copyright (C) 2012 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtSerialPort module of the Qt Toolkit.
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
/* macro definitions */
#define BOOTLOADER_HELLO_STR "\xC1"
#define BOOTLOADER_OK 0x4B
#define BOOTLOADER_PLACEMENT 1

#define PIC_ID 0xD4
#define PIC_FLASHSIZE 0xAC00

#define PIC_NUM_PAGES 42
#define PIC_NUM_ROWS_IN_PAGE  8
#define PIC_NUM_WORDS_IN_ROW 64

#define PIC_WORD_SIZE  (3)
#define PIC_ROW_SIZE  (PIC_NUM_WORDS_IN_ROW * PIC_WORD_SIZE)
#define PIC_PAGE_SIZE (PIC_NUM_ROWS_IN_PAGE  * PIC_ROW_SIZE)

#define PIC_ROW_ADDR(p,r)		(((p) * PIC_PAGE_SIZE) + ((r) * PIC_ROW_SIZE))
#define PIC_WORD_ADDR(p,r,w)	(PIC_ROW_ADDR(p,r) + ((w) * PIC_WORD_SIZE))
#define PIC_PAGE_ADDR(p)		(PIC_PAGE_SIZE * (p))

#define ADDR_24 0
#define ADDR_16 1
#define ADDR_8 2
#define COMMAND_OFFSET 3
#define LENGTH_OFFSET 4
#define PAYLOAD_OFFSET 5
#define HEADER_LENGTH PAYLOAD_OFFSET

#include "programthread.h"

#include <QSerialPort>
#include <QTime>
#include <QtDebug>

programthread::programthread(QObject *parent) :
    QThread(parent)
{
}

programthread::~programthread()
{
    m_mutex.lock();
    m_quit = true;
    m_cond.wakeOne();
    m_mutex.unlock();
    wait();
}

void programthread::transaction(const QString &portName, QByteArray &firmware)
{

    const QMutexLocker locker(&m_mutex);
    m_portName = portName;
    m_firmware=firmware;

    if (!isRunning())
        start();
    else
        m_cond.wakeOne();
}




QByteArray programthread::sendCommandAndWaitForResponse(QByteArray currentRequest){
    QByteArray responseData;
    m_serial->write(currentRequest);
    if (m_serial->waitForBytesWritten(10000)) {
        // read response
        if (m_serial->waitForReadyRead(1000)) {
            responseData = m_serial->readAll();
            while (m_serial->waitForReadyRead(10))
                responseData += m_serial->readAll();
        } else {
            emit timeout(tr("Wait read response timeout %1")
                         .arg(QTime::currentTime().toString()));
        }
    } else {
        emit timeout(tr("Wait write request timeout %1")
                     .arg(QTime::currentTime().toString()));
    }
    return responseData;
}

quint8 programthread::makeCrc(QByteArray buf)
{
    quint8 crc = 0, i = 0;

    for(i=0; i<buf.size(); ++i){
        crc -= (quint8)buf[i];
    }

    return crc;
}


void programthread::run()
{
    bool currentPortNameChanged = false;

    qDebug()<<"From work thread: "<<currentThreadId();

    m_mutex.lock();

    QString currentPortName;
    if (currentPortName != m_portName) {
        currentPortName = m_portName;
        currentPortNameChanged = true;
    }

    //int currentWaitTimeout = m_waitTimeout;
    QByteArray firmware = m_firmware;
    m_mutex.unlock();
    //QSerialPort serial;
    m_serial = new QSerialPort;

    if (currentPortName.isEmpty()) {
        emit error(tr("No port name specified"));
        return;
    }

    int		res = -1;
    quint8		g_verbose = 0;
    quint8		g_hello_only = 0;
    quint8		g_simulate = 0;


    while (!m_quit) {
        if (currentPortNameChanged) {
            m_serial->close();
            m_serial->setPortName(currentPortName);
            m_serial->setBaudRate(QSerialPort::Baud115200);
            m_serial->setDataBits(QSerialPort::Data8);
            m_serial->setParity(QSerialPort::NoParity);
            m_serial->setStopBits(QSerialPort::OneStop);
            m_serial->setFlowControl(QSerialPort::NoFlowControl);

            if (!m_serial->open(QIODevice::ReadWrite)) {
                emit error(tr("Can't open %1, error code %2")
                           .arg(m_portName).arg(m_serial->error()));
                return;
            }
        }

        emit info(tr("Sending Hello to the Bootloader...\n"));

        //send HELLO
        QByteArray currentRequest;
        currentRequest.append(BOOTLOADER_HELLO_STR);
        QByteArray responseData=sendCommandAndWaitForResponse(currentRequest);

        qDebug() << responseData;

        if( responseData.size() != 4 || responseData.at(3) != BOOTLOADER_OK ) {
            emit error(tr("Bootloader did not reply."));
        }

        if( (unsigned char)responseData.at(0) != PIC_ID ) {
            emit error(tr("Wrong chip ID."));
        }

        emit info(tr("OK\n"));

       qint32 res=sendFirmware(m_firmware);

       if( res > 0 ) {
           emit info("\nFirmware updated successfully :)!");
       } else {
           emit info("\nError updating firmware :(");
       }

        m_mutex.lock();
        m_cond.wait(&m_mutex);
        if (currentPortName != m_portName) {
            currentPortName = m_portName;
            currentPortNameChanged = true;
        } else {
            currentPortNameChanged = false;
        }
        //currentWaitTimeout = m_waitTimeout;
        //currentRequest = m_request;
        firmware = m_firmware;
        m_mutex.unlock();
    }

}

qint32 programthread::sendFirmware(QByteArray firmware){

    quint32 u_addr;
    quint32 page  = 0;
    qint32 done  = 0;
    quint32 row   = 0;

    quint32 prog, denom;

    QByteArray command;

    qDebug()<<"Worker::sendFirmware get called from?: "<<QThread::currentThreadId();

    for( page=0; page<PIC_NUM_PAGES; page++)
    {
        u_addr = page * ( PIC_NUM_WORDS_IN_ROW * 2 * PIC_NUM_ROWS_IN_PAGE );
        if( u_addr >= PIC_FLASHSIZE ) {
            emit error(tr("Address out of flash\n"));
            return -1;
        }

        //erase page
        command.clear();
        command.append( (char)((u_addr & 0x00FF0000) >> 16) );
        command.append( (char)((u_addr & 0x0000FF00) >>  8) );
        command.append( (char)((u_addr & 0x000000FF) >>  0));
        command.append( 0x01); //erase command
        command.append( 0x01); //1 byte, CRC
        command.append( (char) makeCrc(command) );


        //if( g_verbose ) {
            //dumpHex(command, HEADER_LENGTH + command[LENGTH_OFFSET]);
        //}

        emit info(QString("Erasing page %1, %2...").arg(page).arg(u_addr,1,16).toLatin1());

        QByteArray responseData=sendCommandAndWaitForResponse(command);
        //qDebug()<<responseData;

        if(responseData.size()==0 || responseData.at(0)!=BOOTLOADER_OK){
               emit info("failed!\n");
                return -1; //return (uint32_t)-1;
        }
        emit info("ok\n");

        //if( g_simulate == 0 && sendCommandAndWaitForResponse(command) < 0 ) {
        //    return -1;
        //}

        //write 8 rows
        for( row = 0; row < PIC_NUM_ROWS_IN_PAGE; row ++, u_addr += (PIC_NUM_WORDS_IN_ROW * 2))
        {
            command.clear();
            command.append( (char) ((u_addr & 0x00FF0000) >> 16 ));
            command.append( (char) ((u_addr & 0x0000FF00) >>  8 ));
            command.append( (char) ((u_addr & 0x000000FF) >>  0));
            command.append( 0x02); //write command
            command.append( char(PIC_ROW_SIZE + 0x01)); //DATA_LENGTH + CRC
            command.append(firmware.mid((int)PIC_ROW_ADDR(page, row),PIC_ROW_SIZE));
            command.append( (char) makeCrc(command));

            emit info(QString("Writing page %1 row %2, %3...").arg(page).arg(row+page*PIC_NUM_ROWS_IN_PAGE).arg(u_addr,1,16).toLatin1());

            QByteArray responseData=sendCommandAndWaitForResponse(command);
            //qDebug()<<responseData;

            if(responseData.size()==0 || responseData.at(0)!=BOOTLOADER_OK){
                  emit info("failed!\n");
                  return -1;
            }
            emit info("ok\n");

            //if( g_verbose ) {
                //dumpHex(command, HEADER_LENGTH + command[LENGTH_OFFSET]);
            //}
            done += PIC_ROW_SIZE;
        }

        prog=((page+1)*100)/PIC_NUM_PAGES;
        qDebug() << prog;
        emit progress((quint8)prog);

    }

    return done;

}



