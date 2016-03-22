#include "sbs1.h"

SBS1::SBS1(QObject *parent) : Tcpserver(parent)
{
//
}

void SBS1::DownlinkBasicReportGroupSlot(DownlinkBasicReportGroup &message)
{
    //MSG,3,,,AES,,,,,,Call,ALT(int),,,LAT(float),LONG(float),,,0,0,0,0
    QByteArray ba=(((QString)"").sprintf("MSG,3,,,%06X,,,,,,%s,%d,,,%f,%f,,,0,0,0,0\n",message.AESID,message.downlinkheader.flightid.toLatin1().data(),qRound(message.altitude),message.latitude,message.longitude)).toLatin1();
    SendBAToAllTCPClients(ba);
}

void SBS1::DownlinkEarthReferenceGroupSlot(DownlinkEarthReferenceGroup &message)
{
    if(!message.truetrack_isvalid)return;
    //MSG,3,,,AES,,,,,,Call,,GroundSpeed(int),TrueTrack(int),,,VerticalRate(int),,0,0,0,0
    QByteArray ba=(((QString)"").sprintf("MSG,3,,,%06X,,,,,,%s,,%d,%d,,,%d,,0,0,0,0\n",message.AESID,message.downlinkheader.flightid.toLatin1().data(),qRound(message.groundspeed),qRound(message.truetrack),qRound(message.verticalrate))).toLatin1();
    SendBAToAllTCPClients(ba);
}


void SBS1::DownlinkGroupsSlot(DownlinkGroups &groups)
{
    if(!groups.isValid())return;
    double ts_min=qFloor(groups.adownlinkbasicreportgroup.time_stamp/60.0);
    double ts_sec=qFloor(groups.adownlinkbasicreportgroup.time_stamp-ts_min*60.0);
    double ts_ms=qFloor((groups.adownlinkbasicreportgroup.time_stamp-ts_min*60.0-ts_sec)*1000.0);
    QDateTime now=QDateTime::currentDateTimeUtc();
    QDateTime ts=now;
    ts.setTime(QTime(now.time().hour(),ts_min,ts_sec,ts_ms));
    if(ts.secsTo(now)<-1800)ts=ts.addSecs(-3600);
    if(ts.secsTo(now)>1800)ts=ts.addSecs(3600);


    if(qAbs(ts.secsTo(now))>900)//15mins
    {
        qDebug()<<"Time way out. Check your clock. Dropping packet";
        return;
    }
    //MSG,3,5,276,4010E9,10088,[2008/11/28,14:53:49.986,2008/11/28,14:58:51.153],,28000,,,53.02551,-2.91389,,,0,0,0,0
    //2008/11/28,14:58:51.153,2008/11/28,14:58:51.153 (ts,now)
    QByteArray datesandtimestr=((QString)ts.toString("yyyy/MM/dd,hh:mm:ss.zzz")+","+now.toString("yyyy/MM/dd,hh:mm:ss.zzz")).toLatin1();
    //qDebug()<<datesandtimestr;

    /*
    //seperated packets
    if(groups.adownlinkbasicreportgroup.valid)
    {
        //MSG,3,,,AES,,[ts_date,ts_time,now_date,now_time],Call,ALT(int),,,LAT(float),LONG(float),,,0,0,0,0
        QByteArray ba=(((QString)"").sprintf("MSG,3,,,%06X,,%s,%s,%d,,,%f,%f,,,0,0,0,0\n",groups.adownlinkbasicreportgroup.AESID,datesandtimestr.data(),groups.adownlinkbasicreportgroup.downlinkheader.flightid.toLatin1().data(),qRound(groups.adownlinkbasicreportgroup.altitude),groups.adownlinkbasicreportgroup.latitude,groups.adownlinkbasicreportgroup.longitude)).toLatin1();
        SendBAToAllTCPClients(ba);
        //qDebug()<<ba;
    }

    if(groups.adownlinkearthreferencegroup.valid&&groups.adownlinkearthreferencegroup.truetrack_isvalid)
    {
        //MSG,3,,,AES,,[ts_date,ts_time,now_date,now_time],Call,,GroundSpeed(int),TrueTrack(int),,,VerticalRate(int),,0,0,0,0
        QByteArray ba=(((QString)"").sprintf("MSG,3,,,%06X,,%s,%s,,%d,%d,,,%d,,0,0,0,0\n",groups.adownlinkearthreferencegroup.AESID,datesandtimestr.data(),groups.adownlinkearthreferencegroup.downlinkheader.flightid.toLatin1().data(),qRound(groups.adownlinkearthreferencegroup.groundspeed),qRound(groups.adownlinkearthreferencegroup.truetrack),qRound(groups.adownlinkearthreferencegroup.verticalrate))).toLatin1();
        SendBAToAllTCPClients(ba);
        //qDebug()<<ba;
    }*/

    //combined packets
    if(groups.adownlinkbasicreportgroup.valid)
    {
        if(groups.adownlinkearthreferencegroup.valid&&groups.adownlinkearthreferencegroup.truetrack_isvalid)
        {
            //MSG,3,,,AES,,[ts_date,ts_time,now_date,now_time],Call,ALT(int),GroundSpeed(int),TrueTrack(int),LAT(float),LONG(float),VerticalRate(int),,0,0,0,0
            QByteArray ba=(((QString)"").sprintf("MSG,3,,,%06X,,%s,%s,%d,%d,%d,%f,%f,%d,,0,0,0,0\n",groups.adownlinkearthreferencegroup.AESID,datesandtimestr.data(),groups.adownlinkearthreferencegroup.downlinkheader.flightid.toLatin1().data(),qRound(groups.adownlinkbasicreportgroup.altitude),qRound(groups.adownlinkearthreferencegroup.groundspeed),qRound(groups.adownlinkearthreferencegroup.truetrack),groups.adownlinkbasicreportgroup.latitude,groups.adownlinkbasicreportgroup.longitude,qRound(groups.adownlinkearthreferencegroup.verticalrate))).toLatin1();
            SendBAToAllTCPClients(ba);
            //qDebug()<<ba;
        }
         else
         {

            //MSG,3,,,AES,,[ts_date,ts_time,now_date,now_time],Call,ALT(int),,,LAT(float),LONG(float),,,0,0,0,0
            QByteArray ba=(((QString)"").sprintf("MSG,3,,,%06X,,%s,%s,%d,,,%f,%f,,,0,0,0,0\n",groups.adownlinkbasicreportgroup.AESID,datesandtimestr.data(),groups.adownlinkbasicreportgroup.downlinkheader.flightid.toLatin1().data(),qRound(groups.adownlinkbasicreportgroup.altitude),groups.adownlinkbasicreportgroup.latitude,groups.adownlinkbasicreportgroup.longitude)).toLatin1();
            SendBAToAllTCPClients(ba);
            //qDebug()<<ba;

         }
    }


}

