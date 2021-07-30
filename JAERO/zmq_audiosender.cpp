#include "zmq_audiosender.h"
#include "zmq.h"
#include <QDebug>

ZMQAudioSender::ZMQAudioSender(QObject *parent) : QObject(parent)
{
    zmqStatus = -1;
    context = zmq_ctx_new();
    publisher = zmq_socket(context, ZMQ_PUB);

    int keepalive = 1;
    int keepalivecnt = 10;
    int keepaliveidle = 1;
    int keepaliveintrv = 1;

    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE,(void*)&keepalive, sizeof(ZMQ_TCP_KEEPALIVE));
    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_CNT,(void*)&keepalivecnt,sizeof(ZMQ_TCP_KEEPALIVE_CNT));
    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_IDLE,(void*)&keepaliveidle,sizeof(ZMQ_TCP_KEEPALIVE_IDLE));
    zmq_setsockopt(publisher, ZMQ_TCP_KEEPALIVE_INTVL,(void*)&keepaliveintrv,sizeof(ZMQ_TCP_KEEPALIVE_INTVL));
}

void ZMQAudioSender::Start(QString &address, QString &topic)
{
    Stop();
    // bind socket
    this->topic=topic;
    connected_url=address.toUtf8().constData();
    zmqStatus=zmq_bind(publisher, connected_url.c_str() );
}

void ZMQAudioSender::Stop()
{
    if(zmqStatus == 0)
    {
        zmqStatus = zmq_disconnect(publisher, connected_url.c_str());
    }
}

void ZMQAudioSender::Voiceslot(QByteArray &data, QString &hex)
{
    std::string topic_text = topic.toUtf8().constData();
    zmq_setsockopt(publisher, ZMQ_IDENTITY, topic_text.c_str(), topic_text.length());

    if(data.length()!=0)
    {
        zmq_send(publisher, topic_text.c_str(), topic_text.length(), ZMQ_SNDMORE);
        zmq_send(publisher, data.data(), data.length(), 0 );
    }
    zmq_send(publisher, topic_text.c_str(), 5, ZMQ_SNDMORE);
    zmq_send(publisher, hex.toStdString().c_str(), 6, 0 );
}
