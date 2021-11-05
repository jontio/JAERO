#include "zmq_audioreceiver.h"
#include "QDebug"
#include <unistd.h>
#include <zmq.h>

//this is so I can easily use old streams that some people are still using for testing.
//commenting this out is for the old format that didn't have samplerate info message.
//without this samplerate is always 48000 and wont work for streams that are using anything else.
//10500bps streams are at 48000 and are what I use for testing.
//#define ZMQ_HAS_SAMPLERATE_MESSAGE

void ZMQAudioReceiver::Start(QString address, QString topic)
{
    //stop set prams and start thread
    Stop();
    setParameters(address,topic);
    future = QtConcurrent::run([=]() {
        process();
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

void ZMQAudioReceiver::Stop()
{
    running = false;
    if(!future.isFinished())future.waitForFinished();
    emit finished();
}

void ZMQAudioReceiver::process()
{
    // allocate enough for 96Khz sampling with 1 buffer per second
    int recsize = 192000;
    context = zmq_ctx_new();
    subscriber = zmq_socket(context, ZMQ_SUB);

    zmqStatus = zmq_connect(subscriber, _address.toStdString().c_str());
    zmq_setsockopt(subscriber, ZMQ_SUBSCRIBE, _topic.toStdString().c_str(), 5);

    char buf [recsize];
    unsigned char rate[4];
    quint32 sampleRate=48000;
    int received;

    running = true;

    while(running)
    {

        //blocking call alternative for topic, here we wait for the topic.
        //I guess the sleep has to be less then the idle period between messages???
        while(((received = zmq_recv(subscriber,  nullptr, 0, ZMQ_DONTWAIT))<0)&&running)
        {
            usleep(10000);
        }
        if(!running)break;

        //rate message is next
#ifdef ZMQ_HAS_SAMPLERATE_MESSAGE
        received = zmq_recv(subscriber, rate, 4, ZMQ_DONTWAIT);
        if(!running)break;
        memcpy(&sampleRate, rate, 4);
#endif

        //then audio data
        received = zmq_recv(subscriber, buf, recsize, ZMQ_DONTWAIT);
        if(!running)break;
        if(received>=0)
        {
            QByteArray qdata(buf, received);
            emit recAudio(qdata,sampleRate);
        }

    }

    if(subscriber)zmq_close(subscriber);
    if(context)zmq_ctx_destroy(context);
    subscriber=nullptr;
    context=nullptr;
}

ZMQAudioReceiver::ZMQAudioReceiver(QObject *parent):
    QObject(parent),
    running(false),
    context(nullptr),
    subscriber(nullptr)
{

}

ZMQAudioReceiver::~ZMQAudioReceiver()
{
    Stop();
}

void ZMQAudioReceiver::setParameters(QString address, QString topic)
{
    _address = address;
    _topic = topic;
}


