#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QHostAddress>
#include <QTimer>

class Tcpclient : public QTcpSocket
{
    Q_OBJECT
public:
    Tcpclient(QObject * parent = 0);
    ~Tcpclient();
    void startclient(const QHostAddress &address = QHostAddress::LocalHost, quint16 port = 30003);
    void stopclient();
public slots:
    void SendBAToTCPServer(QByteArray &ba);
protected:
private:
    QHostAddress currentaddress;
    quint16 currentport;
    QTimer *connectiontimer;
    QAbstractSocket::SocketState socketState;
private slots:
    void connectiontimerslot();
    void stateChangedSlot(QAbstractSocket::SocketState _socketState);
};

#endif // TCPCLIENT_H
