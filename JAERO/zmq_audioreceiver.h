#ifndef ZMQ_AUDIORECEIVER_H
#define ZMQ_AUDIORECEIVER_H

#include <QObject>
#include <QByteArray>
#include <QtConcurrent/QtConcurrent>

class ZMQAudioReceiver : public QObject
{
    Q_OBJECT

public:

    explicit ZMQAudioReceiver(QObject *parent = 0);
    ~ZMQAudioReceiver();

public slots:

    void Stop();
    void Start(QString address, QString topic);

signals:

    void finished();

private:

    void setParameters(QString address, QString topic);
    volatile bool running;

    void process();

    void * volatile context;
    void * volatile subscriber;

    int zmqStatus;

    QString _address;
    QString _topic;
    int _rate;

    QFuture<void> future;

signals:
    void recAudio(const QByteArray & audio,quint32 sampleRate);

};

#endif // AUDIORECEIVER_H
