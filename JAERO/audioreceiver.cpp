#include "audioreceiver.h"
#include "QDebug"
#include <iostream>
#include "QDataStream"
#include "QDateTime"


void AudioReceiver::process()
{
   running = true;
   // allocate enough for 96Khz sampling with 1 buffer per second
   int recsize = 192000;
   context = zmq_ctx_new();
   subscriber = zmq_socket(context, ZMQ_SUB);

   zmqStatus = zmq_connect(subscriber, _address.toStdString().c_str());
   zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, _topic.toStdString().c_str(), 5);

   char buf [recsize];
   char topic[20];

   while(running){


       zmq_recv(subscriber, topic, 20, 0);
       int received = zmq_recv(subscriber, buf, recsize, ZMQ_DONTWAIT);

       QByteArray qdata(buf, received);

       emit recAudio(qdata);
   }

   zmq_disconnect(subscriber,_address.toStdString().c_str());
   zmq_ctx_destroy (context);

   emit finished();

}


AudioReceiver::AudioReceiver(QString address, QString topic){
      _address = address;
      _topic = topic;
}

void AudioReceiver::setParameters(QString address, QString topic){
      _address = address;
      _topic = topic;
}

void AudioReceiver::ZMQaudioStop()
{

running = false;

}
