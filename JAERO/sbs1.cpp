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

