#include "tcpserver.h"

//Jonti 2016

//----------------------
//one socket per client
//----------------------
TcpSocketCustom::TcpSocketCustom(QObject *parent)
    : QTcpSocket(parent)
{
    connect(this,SIGNAL(disconnected()),this,SLOT(ClientDisconnected()));
    connect(this,SIGNAL(readyRead()),this,SLOT(dataavail()));
}
TcpSocketCustom::~TcpSocketCustom()
{
    disconnectFromHost();
}
void TcpSocketCustom::Disconnect()
{
    disconnectFromHost();
}
void TcpSocketCustom::ClientDisconnected()
{
//    qDebug("Client disconnected");
    deleteLater();
}
void TcpSocketCustom::WriteBAToTCPClient(QByteArray &ba)
{
    write(ba);
}
void TcpSocketCustom::dataavail()
{
    while(bytesAvailable()>0)writeDatagramFromTunnel(readAll());
}
//-------------------

//-------------------------
// the server that listens
//-------------------------

Tcpserver::Tcpserver(QObject *parent)
        : QTcpServer(parent)
{
    currentaddress.setAddress(QHostAddress::Null);
    currentport=0;
}

Tcpserver::~Tcpserver()
{
    stopserver();
    emit quitting();
}

void Tcpserver::incomingConnection(qintptr socketDescriptor)
{
    TcpSocketCustom *tcpSocket= new TcpSocketCustom(this);
    if (!tcpSocket->setSocketDescriptor(socketDescriptor))
    {
        qDebug("cant create client socket");
        return;
    }    
    connect(this,SIGNAL(SendBAToAllTCPClientsSignal(QByteArray&)),tcpSocket,SLOT(WriteBAToTCPClient(QByteArray&)));    
    connect(this,SIGNAL(DisconnectAllClients()),tcpSocket,SLOT(Disconnect()));
//    qDebug("Client connected");
    //tcpSocket->write(((QByteArray)"hello\n"));
}

void Tcpserver::SendBAToAllTCPClients(QByteArray &ba)
{
    emit SendBAToAllTCPClientsSignal(ba);
}

//-----------------
void Tcpserver::startserver(const QHostAddress &address, quint16 port)
{
    if(currentaddress==address&&currentport==port&&isListening())return;
    stopserver();
    if(!listen(address, port))
    {
        qDebug()<<"Cant bind to tcp port"<<port;
        return;
    }
    currentaddress=address;
    currentport=port;
//    qDebug()<<"Bound to tcp port "<<port;
}

void Tcpserver::stopserver()
{
    DisconnectAllClients();
    close();
    currentaddress.setAddress(QHostAddress::Null);
    currentport=0;
}

