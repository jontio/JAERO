#ifndef AUDIORECEIVER_H
#define AUDIORECEIVER_H


#include "audioreceiver.h"

#include <QObject>
#include <QThread>
#include <qbytearray.h>
#include <zmq.h>
#include <QtConcurrent/QtConcurrent>


class AudioReceiver : public QObject
{
    Q_OBJECT

public:

    explicit AudioReceiver(QObject *parent = 0);//QString address, QString topic);
    ~AudioReceiver();

public slots:

    void ZMQaudioStop();
    void ZMQaudioStart(QString address, QString topic);

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
