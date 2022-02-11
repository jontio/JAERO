#include "mqttsubscriber.h"
#include <QCoreApplication>
#include <QDebug>

MqttSubscriber::MqttSubscriber(QObject* parent) : QObject(parent),
    client(nullptr),
    messageId(0),
    subscribeed_to_topic_sucseeded(false)
{

}

MqttSubscriber::~MqttSubscriber()
{
#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::~MqttSubscriber";
#endif
}

void MqttSubscriber::onSslErrors(const QList<QSslError>& errors)
{
    Q_UNUSED(errors)
    // Optionally, set ssl errors you want to ignore. Be careful, because this may weaken security.
    // See QSslSocket::ignoreSslErrors(const QList<QSslError> &) for more information.

    // Investigate the errors here, if you find no serious problems, call ignoreSslErrors()
    // to continue connecting.
#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<errors;
#endif
    if(!client)return;
    client->ignoreSslErrors();
}

void MqttSubscriber::connectToHost(const MqttSubscriber_Settings_Object &settings)
{
    setSettings(settings);
    connectToHost();
}

void MqttSubscriber::disconnectFromHost()
{
    if(!client)return;
    client->disconnectFromHost();
}

void MqttSubscriber::delay(int delay_ms)
{
    QEventLoop loop;
    QTimer t;
    t.connect(&t, &QTimer::timeout, &loop, &QEventLoop::quit);
    t.start(delay_ms);
    loop.exec();
}

void MqttSubscriber::connectToHost()
{

#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::connectToHost";
#endif

    //delete old client
    if(client)
    {
        client->disconnectFromHost();
        client->deleteLater();
    }

    //create a new client with us as the parent
    if(settings.encryption)
    {
        client=new QMQTT::Client(settings.host,settings.port,QSslConfiguration::defaultConfiguration(),false,this);
    }
    else
    {
        client=new QMQTT::Client(QHostAddress(settings.host),settings.port,this);
    }

    //add it to the list
    //not needed but oh well
    client_list.append(client);

    //add the connections
    connect(client, &QMQTT::Client::connected, this, &MqttSubscriber::onConnected);
    connect(client, &QMQTT::Client::subscribed, this, &MqttSubscriber::onSubscribed);
    connect(client, &QMQTT::Client::received, this, &MqttSubscriber::onReceived,Qt::UniqueConnection);
    connect(client, &QMQTT::Client::sslErrors, this, &MqttSubscriber::onSslErrors);
    connect(client, &QMQTT::Client::disconnected, this, &MqttSubscriber::onDisconnected);
    connect(client, &QMQTT::Client::destroyed, this, &MqttSubscriber::onClientDestroyed);

    client->setClientId(settings.clientId);
    client->setUsername(settings.username);
    client->setPassword(settings.password);
    client->setCleanSession(true);
    client->setAutoReconnect(true);

    messageId=0;

    client->connectToHost();

}

void MqttSubscriber::onClientDestroyed(QObject *client)
{
#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onClientDestroyed: client index ="<<client_list.indexOf((QMQTT::Client*)client);
    if(client_list.indexOf((QMQTT::Client*)client)<0)
    {
        qDebug()<<"MqttSubscriber::onClientDestroyed: error can't find client in list";
    }
#endif
    client_list.removeAll((QMQTT::Client*)client);
}

void MqttSubscriber::onDisconnected()
{
#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onDisconnected";
#endif
}

void MqttSubscriber::setSettings(const MqttSubscriber_Settings_Object &settings)
{
    if(this->settings!=settings)
    {
#ifdef QMQTT_DEBUG_SUBSCRIBER
        qDebug()<<"MqttSubscriber::setSettings: settings have changed";
#endif
        this->settings=settings;
        if((client)&&(client->connectionState()!=QMQTT::STATE_DISCONNECTED))
        {
#ifdef QMQTT_DEBUG_SUBSCRIBER
            qDebug()<<"MqttSubscriber::setSettings: reconnecting to host with new settings";
#endif
            connectToHost();
        }
    }
}

void MqttSubscriber::onConnected()
{
#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::connected";
#endif
    subscribeed_to_topic_sucseeded=false;
    onSubscribeTimeout();
}

void MqttSubscriber::onSubscribed(const QString& topic)
{
#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::onSubscribed:"<<topic;
    if((settings.topic==topic)&&(settings.subscribe))
    {
        subscribeed_to_topic_sucseeded=true;
#ifdef QMQTT_DEBUG_SUBSCRIBER
        qDebug()<<"MqttSubscriber::onSubscribed: subscribed to wanted topic";
#endif
    }
#endif
}

void MqttSubscriber::onSubscribeTimeout()
{
    if(
            (client)&&
            (settings.subscribe)&&
            (!subscribeed_to_topic_sucseeded)&&
            ((client->connectionState()==QMQTT::STATE_CONNECTED))
      )
    {
#ifdef QMQTT_DEBUG_SUBSCRIBER
        qDebug()<<"MqttSubscriber::onSubscribeTimeout: tring to subscribe to"<<settings.topic;
#endif
        client->subscribe(settings.topic, 0);
        QTimer::singleShot(QMQTT_SUBSCRIBE_TIME_IN_MS,this,SLOT(onSubscribeTimeout()));
    }
}

void MqttSubscriber::onReceived(const QMQTT::Message& message)
{
//#ifdef QMQTT_DEBUG_SUBSCRIBER
//    qDebug()<<"MqttSubscriber::onReceived: receiving a message";
//#endif
    if(!settings.subscribe)return;
    QByteArray ba=qUncompress(message.payload());
    if(!aco.fromQByteArray(ba))
    {
#ifdef QMQTT_DEBUG_SUBSCRIBER
        qDebug()<<"received message looks bad. ditching";
#endif
        qDebug()<<message.payload();
        return;
    }
#ifdef QMQTT_DEBUG_SUBSCRIBER
    else qDebug()<<"received good message";
#endif
    emit ACARSsignal(aco.toACARSItem());
}

void MqttSubscriber::ACARSslot(ACARSItem &acarsitem)
{
    if(!settings.publish)return;
#ifdef QMQTT_DEBUG_SUBSCRIBER
    qDebug()<<"MqttSubscriber::ACARSslot: sending a message";
#endif
    aco=acarsitem;
    QByteArray ba=qCompress(aco,9);
    QMQTT::Message message(messageId, settings.topic,ba);
    client->publish(message);
    messageId++;
}
