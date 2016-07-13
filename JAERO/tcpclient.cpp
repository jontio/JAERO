#include "tcpclient.h"
//#include <QDebug>

Tcpclient::Tcpclient(QObject *parent)
    : QTcpSocket(parent)
{
    connectiontimer = new QTimer(this);
    connect(connectiontimer, SIGNAL(timeout()), this, SLOT(connectiontimerslot()));
    socketState=UnconnectedState;
    connect(this,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChangedSlot(QAbstractSocket::SocketState)));
    stopclient();
    currentaddress.setAddress(QHostAddress::Null);
    currentport=0;
}

Tcpclient::~Tcpclient()
{
    stopclient();
}

void Tcpclient::stopclient()
{
    connectiontimer->stop();
    abort();
    close();
}

void Tcpclient::startclient(const QHostAddress &address, quint16 port)
{
    if(currentaddress==address&&currentport==port&&socketState==ConnectedState)
    {
//        qDebug()<<"all ready connected";
        return;
    }
    stopclient();
    if(address==QHostAddress::Null || port==0)
    {
        currentaddress.setAddress(QHostAddress::Null);
        currentport=0;
        return;
    }
//    qDebug()<<"trying to connect to port"<<address<<port;
    connectToHost(address, port);
    currentaddress=address;
    currentport=port;
    connectiontimer->start(30000);
}

void Tcpclient::SendBAToTCPServer(QByteArray &ba)
{
    if(!isOpen())return;
    write(ba);
}

void Tcpclient::connectiontimerslot()
{
//    qDebug()<<"connectiontimerslot";
    if(socketState==ConnectedState)return;
    startclient(currentaddress,currentport);
}

void Tcpclient::stateChangedSlot(QAbstractSocket::SocketState _socketState)
{
    socketState=_socketState;
//    qDebug()<<socketState;
}

