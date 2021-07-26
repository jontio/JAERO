#ifndef ZMQ_AUDIOSENDER_H
#define ZMQ_AUDIOSENDER_H

#include <QObject>

class ZMQAudioSender : public QObject
{
    Q_OBJECT
public:
    explicit ZMQAudioSender(QObject *parent = 0);
    void Start(QString &address,QString &topic);
    void Stop();
signals:
public slots:
    void Voiceslot(QByteArray &data, QString &hex);
private:
    void* context;
    void* publisher;
    int zmqStatus;
    std::string connected_url;
    QString topic;
};

#endif // ZMQ_AUDIOSENDER_H
