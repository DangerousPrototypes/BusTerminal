#ifndef LOADIMAGE_H
#define LOADIMAGE_H

#include <QObject>
#include <QtSerialPort>

class loadImageWorker: public QObject
{
    Q_OBJECT

public:
    explicit loadImageWorker(const QString &portName, const QString &fileName);
    ~loadImageWorker();

public slots:
    void load();

signals:
    void response(const QString &s);
    void error(const QString &s);
    void timeout(const QString &s);
    void info(const QString &s);
    void progress(const quint8 &c);
    void finished();

private:
    quint8 makeCrc(QByteArray buf);
    QByteArray sendCommandAndWaitForResponse(QByteArray currentRequest, quint16 delay);
    qint32 sendFirmware(QByteArray firmware);

    QString m_portName;
    QString m_filename;
    QSerialPort *m_serial = nullptr;


};

#endif // LOADICE40_H
