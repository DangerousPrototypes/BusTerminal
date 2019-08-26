#ifndef LOADERINTELHEX_H
#define LOADERINTELHEX_H

#include <QObject>

class loadhexworker: public QObject
{
    Q_OBJECT

public:
    loadhexworker(const QString &fileName);
    ~loadhexworker();

public slots:
    void load();

signals:
    void info(const QString &info);
    void error(const QString &error);
    void output(const QByteArray &firmware);
    void finished();

private:
    QString m_filename;


};

#endif // LOADERINTELHEX_H
