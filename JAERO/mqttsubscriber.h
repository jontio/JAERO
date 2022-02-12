#ifndef MQTTSUBSCRIBER_H
#define MQTTSUBSCRIBER_H

#include <QObject>
#include "qmqtt.h"
#include "acarsitem_converter.h"

//#define QMQTT_DEBUG_SUBSCRIBER

const QString    QMQTT_DEFAULT_HOST = "example.com";
const quint16    QMQTT_DEFAULT_PORT = 8883;
const QString    QMQTT_DEFAULT_TOPIC = "acars/jaero";
const bool       QMQTT_DEFAULT_ENCRYPTION = true;
const QString    QMQTT_DEFAULT_CLIENTID = "changeme_id_make_random";
const QString    QMQTT_DEFAULT_USERNAME = "changeme_unsername";
const QByteArray QMQTT_DEFAULT_PASSWORD = "changeme_password";
const bool       QMQTT_DEFAULT_SUBSCRIBE = true;
const bool       QMQTT_DEFAULT_PUBLISH = false;

#define QMQTT_SUBSCRIBE_TIME_IN_MS 10000

class MqttSubscriber_Settings_Object : public JSerialize
{
    Q_OBJECT
public:
    J_SERIALIZE(QString,host,QMQTT_DEFAULT_HOST)
    J_SERIALIZE(quint16,port,QMQTT_DEFAULT_PORT)
    J_SERIALIZE(QString,topic,QMQTT_DEFAULT_TOPIC)
    J_SERIALIZE(bool,encryption,QMQTT_DEFAULT_ENCRYPTION)
    J_SERIALIZE(QString,clientId,QMQTT_DEFAULT_CLIENTID)
    J_SERIALIZE(QString,username,QMQTT_DEFAULT_USERNAME)
    J_SERIALIZE(QByteArray,password,QMQTT_DEFAULT_PASSWORD)
    J_SERIALIZE(bool,subscribe,QMQTT_DEFAULT_SUBSCRIBE)
    J_SERIALIZE(bool,publish,QMQTT_DEFAULT_PUBLISH)
};

class MqttSubscriber : public QObject
{
    Q_OBJECT
public:

    //has QMQTT::ConnectionState enum is a subset of this
    enum ConnectionState
    {
        STATE_INIT = 0,
        STATE_CONNECTING,
        STATE_CONNECTED,
        STATE_DISCONNECTED,
        STATE_CONNECTED_SUBSCRIBED=0xFF
    };
    Q_ENUM(ConnectionState)

    explicit MqttSubscriber(QObject* parent = NULL);
    virtual ~MqttSubscriber();
public slots:
    void setSettings(const MqttSubscriber_Settings_Object &settings);
    void connectToHost(const MqttSubscriber_Settings_Object &settings);
    void connectToHost();
    void disconnectFromHost();
    void ACARSslot(ACARSItem &acarsitem);
signals:
    void ACARSsignal(ACARSItem &acarsitem);
    void connectionStateChange(MqttSubscriber::ConnectionState state);
private:
    ACARSItem_QObject aco;
    MqttSubscriber_Settings_Object settings;
    QMQTT::Client *client;
    void delay(int delay_ms);
    QList<QMQTT::Client *> client_list;
    int messageId;

    //if you subscribe to something you don't
    //have access to m_lastSubscriptionState
    //still gets set.
    bool m_lastSubscriptionState;
    QMQTT::ConnectionState m_lastClientConnectionState;

    void updateState(bool subscriptionState);
    void updateState();

private slots:
    void onConnected();
    void onSubscribed(const QString& topic);
    void onReceived(const QMQTT::Message& message);
    void onSslErrors(const QList<QSslError>& errors);
    void onDisconnected();
    void onClientDestroyed(QObject * = nullptr);
    void onSubscribeTimeout();
    void onError(const QMQTT::ClientError error);
    void onUnsubscribed(const QString& topic);
};

#endif // MQTTSUBSCRIBER_H
