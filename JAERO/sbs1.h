#ifndef SBS1_H
#define SBS1_H

#include <QObject>
#include "tcpserver.h"
#include "tcpclient.h"
#include "arincparse.h"

class SBS1 : public QObject
{
    Q_OBJECT
public:
    explicit SBS1(QObject *parent = 0);
    void starttcpconnection(const QHostAddress &address, quint16 port, bool behaveasclient);
    void stoptcpconnection();
signals:
    void SendBAViaTCP(QByteArray &ba);
public slots:
    void DownlinkBasicReportGroupSlot(DownlinkBasicReportGroup &message);
    void DownlinkEarthReferenceGroupSlot(DownlinkEarthReferenceGroup &message);
    void DownlinkGroupsSlot(DownlinkGroups &groups);
private:
    Tcpserver *tcpserver;
    Tcpclient *tcpclient;
    bool running;
};

#endif // SBS1_H
