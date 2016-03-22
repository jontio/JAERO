#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <QTcpServer>
#include <QTcpSocket>


//Jonti 2016

class TcpSocketCustom: public QTcpSocket
{
    Q_OBJECT
public:
    TcpSocketCustom(QObject * parent = 0);
    ~TcpSocketCustom();
public slots:
    void ClientDisconnected();
    void WriteBAToTCPClient(QByteArray &ba);
    void dataavail();
    void Disconnect();
signals:
    void writeDatagramFromTunnel(QByteArray ba);
private:
protected:
};

class Tcpserver : public QTcpServer
{
    Q_OBJECT
public:
    Tcpserver(QObject *parent = 0);
    ~Tcpserver();
    void SendBAToAllTCPClients(QByteArray &ba);
    void startserver(const QHostAddress &address = QHostAddress::Any, quint16 port = 0);
    void stopserver();
public slots:
signals:
    void quitting();
    void SendBAToAllTCPClientsSignal(QByteArray &ba);
    void DisconnectAllClients();
protected:
    void incomingConnection(qintptr socketDescriptor);
private:
    QHostAddress currentaddress;
    quint16 currentport;

};

#endif // TCPSERVER_H
