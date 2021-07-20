#ifndef AUDIORECEIVER_H
#define AUDIORECEIVER_H


#include "audioreceiver.h"

#include <QObject>
#include <QThread>
#include <qbytearray.h>
#include <zmq.h>


class AudioReceiver : public QObject
{
    Q_OBJECT

public:

   AudioReceiver(QString address, QString topic);
   void setParameters(QString address, QString topic);

   bool running;


public slots:

   void process();
   void ZMQaudioStop();

signals:

    void finished();


private:

   void* context;
   void* subscriber;

   int zmqStatus;

   QString _address;
   QString _topic;
   int _rate;





signals:
    void recAudio(const QByteArray & audio);

};

#endif // AUDIORECEIVER_H
