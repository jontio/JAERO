#include "audioreceiver.h"
#include "QDebug"
#include <iostream>
#include "QDataStream"
#include "QDateTime"

#include <unistd.h>

void AudioReceiver::ZMQaudioStart(QString address, QString topic)
{
    //stop set prams and start thread
    ZMQaudioStop();
    setParameters(address,topic);
    future = QtConcurrent::run([=]() {
        process();
        qDebug()<<"Thread finished";
        return;
    });
    //wait till the thread is running so ZMQaudioStop functions correctly
    for(int i=0;!running&&i<1000;i++)usleep(1000);
    //if it fails after a second then arghhhhh, should not happen
    if(!running)
    {
        qDebug()<<"Failed to start ZMQ receiving thread";
    }
}

void AudioReceiver::process()
{
    // allocate enough for 96Khz sampling with 1 buffer per second
    int recsize = 192000;
    context = zmq_ctx_new();
    subscriber = zmq_socket(context, ZMQ_SUB);

    zmqStatus = zmq_connect(subscriber, _address.toStdString().c_str());
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, _topic.toStdString().c_str(), 5);

    char buf [recsize];
    char topic[20];

    running = true;

    while(running)
    {
        zmq_recv(subscriber, topic, 20, 0);
        int received = zmq_recv(subscriber, buf, recsize, ZMQ_DONTWAIT);
        if(received>=0)
        {
            QByteArray qdata(buf, received);
            emit recAudio(qdata);
        }
        else
        {
            if(running)
            {
                qDebug()<<"zmq_recv error!!!";
                usleep(100000);
            }
        }
    }
}


AudioReceiver::AudioReceiver(QObject *parent):
    QObject(parent),
    running(false),
    context(nullptr),
    subscriber(nullptr)
{

}

AudioReceiver::~AudioReceiver()
{
    ZMQaudioStop();
}

void AudioReceiver::setParameters(QString address, QString topic)
{
    _address = address;
    _topic = topic;
}

void AudioReceiver::ZMQaudioStop()
{

    running = false;

    if(subscriber)zmq_close(subscriber);
    if(context)zmq_ctx_destroy(context);
    if(!future.isFinished())future.waitForFinished();

    subscriber=nullptr;
    context=nullptr;

    emit finished();

}
