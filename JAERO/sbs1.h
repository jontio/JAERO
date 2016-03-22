#ifndef SBS1_H
#define SBS1_H

#include "tcpserver.h"
#include "arincparse.h"

class SBS1 : public Tcpserver
{
    Q_OBJECT
public:
    explicit SBS1(QObject *parent = 0);

signals:

public slots:
    void DownlinkBasicReportGroupSlot(DownlinkBasicReportGroup &message);
    void DownlinkEarthReferenceGroupSlot(DownlinkEarthReferenceGroup &message);
    void DownlinkGroupsSlot(DownlinkGroups &groups);
private:
    Tcpserver *server;
};

#endif // SBS1_H
