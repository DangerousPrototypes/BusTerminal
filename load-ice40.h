#ifndef LOADICE40_H
#define LOADICE40_H

#include <QObject>
#include <QtSerialPort>

class loadIce40: public QObject
{
    Q_OBJECT

public:
    explicit loadIce40(const QString &portName, const QString &fileName);
    ~loadIce40();

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
