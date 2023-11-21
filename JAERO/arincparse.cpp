#include "arincparse.h"
#include <QDebug>
#include <libacars/libacars.h>
#include <libacars/acars.h>
#include <libacars/vstring.h>

ArincParse::ArincParse(QObject *parent) : QObject(parent)
{
    MetaObject = this->staticMetaObject;
    MetaEnumIMI = MetaObject.enumerator(MetaObject.indexOfEnumerator("IMIEnum"));
    MetaEnumMessageSourcesIDS = MetaObject.enumerator(MetaObject.indexOfEnumerator("MessageSourcesIDS"));
    middlespacer="  ";
}

//
//      ba=
//
// 0 [--------]
// 1 [-S123456]
// 2 [789-----]
// 3 [--------]
//
//to extract signed # with sign bit S. extractqint32(ba,2,5,10,true)
qint32 ArincParse::extractqint32(QByteArray &ba,int lsbyteoffset,int bitoffset,int numberofbits,bool issigned)
{
    numberofbits--;
    qint32 val=0;
    int shift=0;
    qint32 byte;
    assert(bitoffset>=0);
    assert(bitoffset<=8);
    assert(numberofbits>=0);
    int mask=(~((0x00FF)<<(8-bitoffset)))&0xFF;
    for(int i=lsbyteoffset;i>=0;i--)
    {
        if((i-1)>=0)byte=((ba.at(i)>>bitoffset)&mask)|((ba.at(i-1)<<(8-bitoffset))&~mask);
         else byte=((ba.at(i)>>bitoffset)&mask);
        byte&=0xFF;
        val|=(byte<<shift);
        shift+=8;
        if((shift>numberofbits)||(shift>24))break;
    }
    if(issigned)
    {
        if(((val>>numberofbits)&0x00000001)) val|=(0xFFFFFFFF<<(numberofbits+1));
         else val&=~(0xFFFFFFFF<<(numberofbits+1));
    }
     else val&=~(0xFFFFFFFF<<(numberofbits+1));
    return val;
}

void ArincParse::try_acars_apps(ACARSItem &acarsitem, la_msg_dir msg_dir)
{
//    qDebug()<<"void ArincParse::try_acars_apps(ACARSItem &acarsitem, la_msg_dir msg_dir)";
    if(acarsitem.message.isEmpty())return;
//    qDebug()<<"acarsitem="<<acarsitem.message;
//    qDebug()<<"msg_dir="<<msg_dir;
//    qDebug()<<"acarsitem.valid="<<acarsitem.valid;
//    qDebug()<<"arincmessage.info="<<arincmessage.info;
    QByteArray ba="";
    if(msg_dir == LA_MSG_DIR_AIR2GND)
    {
        // skip message number and flight ID
        ba = acarsitem.message.mid(4+6).toLatin1();
    }
    else
    {
        // look for a sublabel and if found strip the sublabel(s) from the message before decoding
        ba = acarsitem.message.toLatin1();
        char sublabel[3];
        char mfi[3];
        int offset = la_acars_extract_sublabel_and_mfi(acarsitem.LABEL.data(), msg_dir, ba.data(),strlen( ba.data()), sublabel, mfi);

        if(offset > 0)
        {
            ba = "/" + acarsitem.message.right(acarsitem.message.length()-offset).replace("- #" + QString(sublabel),"").trimmed().toLatin1();
        }
        else
        {
            ba = acarsitem.message.toLatin1();
        }
    }
    if(ba.isEmpty())return;

    la_proto_node *node = la_acars_decode_apps(acarsitem.LABEL.data(), ba.data(), msg_dir);
    if(node != NULL)
    {
        la_vstring *vstr = la_proto_tree_format_text(NULL, node);
//        qDebug()<<"vstr->str="<<vstr->str;
        arincmessage.info += vstr->str;
        la_vstring_destroy(vstr, true);

        QByteArray arr;

        vstr = la_proto_tree_format_json(NULL, node);
        arr = QString(vstr->str).toUtf8();
        la_vstring_destroy(vstr, true);

        arincmessage.info_json = QJsonDocument::fromJson(arr).object();
    }
    la_proto_tree_destroy(node);

}

bool ArincParse::parseUplinkmessage(ACARSItem &acarsitem)
{
    if(acarsitem.downlink)return false;
    if(acarsitem.nonacars)return false;

    try_acars_apps(acarsitem, LA_MSG_DIR_GND2AIR);
    return true;
}

//returns true if something for the user to read. need to check what is valid on return
bool ArincParse::parseDownlinkmessage(ACARSItem &acarsitem, bool onlyuselibacars)
{
    //qDebug()<<acarsitem.message;

    downlinkgroups.clear();
    arincmessage.clear();
    arincmessage.downlink=acarsitem.downlink;
    downlinkheader.clear();

    if(!acarsitem.downlink)return false;
    if(acarsitem.nonacars)return false;

    //ref 622. 4.3.3 and 745

    //format is header/MFI ctraddr.IMI_tailno_appmessage_CRC
    // or       header/ctraddr.IMI_tailno_app_message_CRC

    //deal with header stuff
    //ref 620 & 618
    if(acarsitem.message.size()<10)return false;
    char Originator=-1;
    Originator=acarsitem.message.at(0).toLatin1();
    downlinkheader.OriginatorString=MetaEnumMessageSourcesIDS.valueToKey(Originator);//the name of the device on the plane that sent the message if anyone is interested
    bool ok;
    downlinkheader.MessageNumber=acarsitem.message.mid(1,2).toInt(&ok);
    if(!ok)return false;
    downlinkheader.BlockSequenceCharacter=acarsitem.message.at(3).toLatin1();
    downlinkheader.flightid=acarsitem.message.mid(4,6);//unsually flight # but not allways
    //remove zero padding in flight id
    QRegExp rx("^[A-Z]*(0*)");
    int pos = rx.indexIn(downlinkheader.flightid);
    if(pos!=-1)downlinkheader.flightid.remove(rx.pos(1),rx.cap(1).size());
    if((downlinkheader.flightid.size()<3)||!downlinkheader.flightid[0].isLetter())downlinkheader.flightid.clear();//lets say not a flight #
    downlinkheader.valid=true;

    //deal with application section
    QStringList sections=acarsitem.message.split('/');
    // Not an ARINC message - try other ACARS applications with libacars
    if(sections.size()!=2)
    {
        try_acars_apps(acarsitem, LA_MSG_DIR_AIR2GND);
        return true;
    }

    //split up into components as strings
    //eg "F58ADL0040/AKLCDYA.ADS.N705DN07EEE19454DAC7D010D21D0DEEEC44556208024029F0588C71D7884D000E13B90F00000F12C1A280001029305F1019F4"
    //becomes...
    //""
    //"AKLCDYA"
    //"ADS"
    //".N705DN"
    //"07EEE19454DAC7D010D21D0DEEEC44556208024029F0588C71D7884D000E13B90F00000F12C1A280001029305F10"
    //"19F4"
    QString MFI_ctraddr=sections[1].section('.',0,0);
    QString IMI_tailno_appmessage_CRC=sections[1].section('.',1);
    QString MFI;
    QString ctraddr=MFI_ctraddr;
    if(MFI_ctraddr.contains(' '))//incase MFI is present (H1 label rather than the MFI label) happens when ACARS peripheral created message rather than by an ACARS [C]MU
    {
        MFI=MFI_ctraddr.section(' ',0,0);// eg B6 if there. its not there in this example as the ACARS header would have come with a B6 in the ACARS label
        ctraddr=MFI_ctraddr.section(' ',1);// eg AKLCDYA
    } else MFI=acarsitem.LABEL;// eg B6
    arincmessage.IMI=IMI_tailno_appmessage_CRC.left(3);//eg ADS
    arincmessage.tailno=IMI_tailno_appmessage_CRC.mid(3,7);//eg .N705DN
    QString appmessage=IMI_tailno_appmessage_CRC.mid(3+7,IMI_tailno_appmessage_CRC.size()-3-7-4);//eg 07EEE19454DAC7D010D21D0DEEEC44556208024029F0588C71D7884D000E13B90F00000F12C1A280001029305F10
    QString CRC=IMI_tailno_appmessage_CRC.right(4);//eg 19F4

    //qDebug()<<MFI;
    //qDebug()<<ctraddr;
    //qDebug()<<arincmessage.IMI;
    //qDebug()<<arincmessage.tailno;
    //qDebug()<<appmessage;
    //qDebug()<<CRC;

    //message into bytes
    QByteArray appmessage_bytes=QByteArray::fromHex(appmessage.toLatin1());

    //CRC to int
    int crc_rec=CRC.toInt(&ok,16);

    //calc crc
    QByteArray adsmessage=arincmessage.IMI.toLatin1()+arincmessage.tailno.toLatin1()+appmessage_bytes;
    int crc_calc=crc16.calcusingbytesotherendines(adsmessage.data(),adsmessage.size());

    //remove . in reg
    arincmessage.tailno.remove('.');

    //fail if crc no good
    if(crc_rec!=crc_calc)
    {
        //qDebug()<<"Application CRC failed"<<crc_rec<<crc_calc;
        return false;
    }
    arincmessage.valid=true;

    if (onlyuselibacars) 
    {
        try_acars_apps(acarsitem, LA_MSG_DIR_AIR2GND);
        return true;
    }

    //switch on IMI
    bool fail=false;

    switch(MetaEnumIMI.keysToValue(arincmessage.IMI.toLatin1()))
    {
    case ADS:
    {
        //qDebug()<<"ADS packet";
        for(int i=0;(i<appmessage_bytes.size())&&(!fail);)
        {
            switch(appmessage_bytes.at(i))
            {
            case Acknowledgement:
            {
                if((i+2)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage.info+="Acknowledgement";
                arincmessage.info+=((QString)" ADS Contract Request Number = %1\n").arg((uchar)appmessage_bytes.at(i-1+2));
                i+=2;//goto next message
                break;
            }
            case Negative_Acknowledgement:
            {
                if((i+4)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage.info+="Negative_Acknowledgement";
                arincmessage.info+=((QString)" ADS Contract Request Number = %1 Reason = %2\n").arg((uchar)appmessage_bytes.at(i-1+2)).arg((((QString)"%1").arg((uchar)appmessage_bytes.at(i-1+3),2, 16, QChar('0'))).toUpper());
                i+=4;//goto next message
                break;
            }
            case Predicted_Route_Group:
            {
                if((i+18)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Predicted_Route_Group";

                //for next waypoint. cant be bothered to do +1 waypoint just the next will do. feel free to add the +1
                //Lat
                double latitude=((double)extractqint32(appmessage_bytes,i-1+4,3,21,true))*lat_scaller;
                //Long
                double longitude=((double)extractqint32(appmessage_bytes,i-1+7,6,21,true))*long_scaller;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+9,6,16,true))*alt_scaller;
                //ETA (is this time till arrival or time at arrival??)
                double eta=((double)extractqint32(appmessage_bytes,i-1+10,0,14,false));

                arincmessage.info+=middlespacer+((QString)"Next waypoint Lat = %1 Long = %2 Alt = %3 feet. ETA = %4\n").arg(latitude).arg(longitude).arg(altitude).arg(QDateTime::fromTime_t(eta).toUTC().toString("hh:mm:ss"));

                i+=18;//goto next message
                break;
            }
            case Meteorological_Group:
            {
                if((i+5)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Meteorological_Group";

                //Wind speed
                double windspeed=((double)extractqint32(appmessage_bytes,i-1+3,7,9,false))*windspeed_scaller;
                //True wind direction
                bool truewinddirection_isvalid=!((((uchar)appmessage_bytes.at(i-1+3))>>6)&0x01);
                double truewinddirection=((double)extractqint32(appmessage_bytes,i-1+4,5,9,true))*truewinddirection_scaller;
                if(truewinddirection<0)truewinddirection+=360.0;
                //Temperature (arcinc 745 seems to make a mistake with their egampe on this one)
                double temperature=((double)extractqint32(appmessage_bytes,i-1+5,1,12,true))*temperature_scaller;

                if(truewinddirection_isvalid)arincmessage.info+=middlespacer+((QString)"Wind speed = %1 knots. True wind direction = %2 deg. Temperature = %3 deg C.\n").arg(qRound(windspeed)).arg(qRound(truewinddirection)).arg(temperature);
                 else arincmessage.info+=middlespacer+((QString)"Wind speed = %1 knots. Temperature = %2 deg C.\n").arg(qRound(windspeed)).arg(temperature);


                i+=5;//goto next message
                break;
            }
            case Air_Reference_Group:
            {
                if((i+6)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Air_Reference_Group";

                //True heading
                bool trueheading_isvalid=!((((uchar)appmessage_bytes.at(i-1+2))>>7)&0x01);
                double trueheading=((double)extractqint32(appmessage_bytes,i-1+3,3,12,true))*trueheading_scaller;
                if(trueheading<0)trueheading+=360.0;
                //Mach speed
                double machspeed=((double)extractqint32(appmessage_bytes,i-1+5,6,13,false))*machspeed_scaller;
                //Vertical rate
                double verticalrate=((double)extractqint32(appmessage_bytes,i-1+6,2,12,true))*verticalrate_scaller;

                if(trueheading_isvalid)arincmessage.info+=middlespacer+((QString)"True heading = %1 deg. Mach speed = %2 Vertical rate = %3 fpm.\n").arg(qRound(trueheading)).arg(qRound(machspeed*100.0)/100.0).arg(verticalrate);
                 else arincmessage.info+=middlespacer+((QString)"Mach speed = %1 Vertical rate = %2 fpm.\n").arg(qRound(machspeed*100.0)/100.0).arg(verticalrate);


                i+=6;//goto next message
                break;
            }
            case Earth_Reference_Group:
            {
                if((i+6)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Earth_Reference_Group";


                //True track
                bool truetrack_isvalid=!((((uchar)appmessage_bytes.at(i-1+2))>>7)&0x01);
                double truetrack=((double)extractqint32(appmessage_bytes,i-1+3,3,12,true))*truetrack_scaller;
                if(truetrack<0)truetrack+=360.0;
                //Ground speed
                double groundspeed=((double)extractqint32(appmessage_bytes,i-1+5,6,13,false))*groundspeed_scaller;
                //Vertical rate
                double verticalrate=((double)extractqint32(appmessage_bytes,i-1+6,2,12,true))*verticalrate_scaller;

                if(truetrack_isvalid)arincmessage.info+=middlespacer+((QString)"True Track = %1 deg. Ground speed = %2 knots. Vertical rate = %3 fpm.\n").arg(qRound(truetrack)).arg(qRound(groundspeed)).arg(verticalrate);
                 else arincmessage.info+=middlespacer+((QString)"Ground speed = %1 knots. Vertical rate = %2 fpm.\n").arg(qRound(groundspeed)).arg(verticalrate);

                //populate group
                downlinkgroups.adownlinkearthreferencegroup.AESID=acarsitem.isuitem.AESID;
                downlinkgroups.adownlinkearthreferencegroup.downlinkheader=downlinkheader;
                downlinkgroups.adownlinkearthreferencegroup.truetrack=truetrack;
                downlinkgroups.adownlinkearthreferencegroup.truetrack_isvalid=truetrack_isvalid;
                downlinkgroups.adownlinkearthreferencegroup.groundspeed=groundspeed;
                downlinkgroups.adownlinkearthreferencegroup.verticalrate=verticalrate;
                downlinkgroups.adownlinkearthreferencegroup.valid=true;

                i+=6;//goto next message
                break;
            }
            case Flight_Identification_Group:
            {
                if((i+7)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Flight_Identification_Group";

                QByteArray ba(8,' ');
                ba[0]=extractqint32(appmessage_bytes,i-1+2,2,6,false);
                ba[1]=extractqint32(appmessage_bytes,i-1+3,4,6,false);
                ba[2]=extractqint32(appmessage_bytes,i-1+4,6,6,false);
                ba[3]=extractqint32(appmessage_bytes,i-1+4,0,6,false);
                ba[4]=extractqint32(appmessage_bytes,i-1+5,2,6,false);
                ba[5]=extractqint32(appmessage_bytes,i-1+6,4,6,false);
                ba[6]=extractqint32(appmessage_bytes,i-1+7,6,6,false);
                ba[7]=extractqint32(appmessage_bytes,i-1+7,0,6,false);
                for(int k=0;k<8;k++)if((uchar)ba[k]<=26)ba[k]=ba[k]|0x40;

                arincmessage.info+=middlespacer+"Flight ID "+ba.trimmed()+"\n";

                i+=7;//goto next message
                break;
            }
            case Basic_Report:
            case Emergency_Basic_Report:
            case Lateral_Deviation_Change_Event:
            case Vertical_Rate_Change_Event:
            case Altitude_Range_Event:
            case Waypoint_Change_Event:
            {
                if((i+11)>appmessage_bytes.size()){fail=true;break;}//sz check
                switch(appmessage_bytes.at(i))
                {
                case Basic_Report:
                    arincmessage.info+="Basic_Report:\n";
                    break;
                case Emergency_Basic_Report:
                    arincmessage.info+="Emergency_Basic_Report:\n";
                    break;
                case Lateral_Deviation_Change_Event:
                    arincmessage.info+="Lateral_Deviation_Change_Event:\n";
                    break;
                case Vertical_Rate_Change_Event:
                    arincmessage.info+="Vertical_Rate_Change_Event:\n";
                    break;
                case Altitude_Range_Event:
                    arincmessage.info+="Altitude_Range_Event:\n";
                    break;
                case Waypoint_Change_Event:
                    arincmessage.info+="Waypoint_Change_Event:\n";
                    break;
                }

                //Lat
                double latitude=((double)extractqint32(appmessage_bytes,i-1+4,3,21,true))*lat_scaller;
                //Long
                double longitude=((double)extractqint32(appmessage_bytes,i-1+7,6,21,true))*long_scaller;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+9,6,16,true))*alt_scaller;
                //Time stamp
                double time_stamp=((double)extractqint32(appmessage_bytes,i-1+11,7,15,false))*time_scaller;
                QTime ts=QDateTime::fromTime_t(qRound(time_stamp)).toUTC().time();
                QString ts_str=(ts.toString("mm")+"m "+ts.toString("ss")+"s");
                //FOM
                int FOM=((uchar)appmessage_bytes.at(i-1+11))&0x1F;

                arincmessage.info+=middlespacer+((QString)"Lat = %1 Long = %2 Alt = %3 feet. Time past the hour = %4 FOM = %5\n").arg(latitude).arg(longitude).arg(altitude).arg(ts_str).arg((((QString)"%1").arg(FOM,2, 16, QChar('0'))).toUpper());

                //populate group
                downlinkgroups.adownlinkbasicreportgroup.AESID=acarsitem.isuitem.AESID;
                downlinkgroups.adownlinkbasicreportgroup.downlinkheader=downlinkheader;
                downlinkgroups.adownlinkbasicreportgroup.messagetype=(ADSDownlinkMessages)appmessage_bytes.at(i);
                downlinkgroups.adownlinkbasicreportgroup.latitude=latitude;
                downlinkgroups.adownlinkbasicreportgroup.longitude=longitude;
                downlinkgroups.adownlinkbasicreportgroup.altitude=altitude;
                downlinkgroups.adownlinkbasicreportgroup.time_stamp=time_stamp;
                downlinkgroups.adownlinkbasicreportgroup.FOM=FOM;
                downlinkgroups.adownlinkbasicreportgroup.valid=true;

                i+=11;//goto next message
                break;
            }
            case Noncompliance_Notification://not fully implimented.
            {
                if((i+6)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage.info+="Noncompliance_Notification";
                arincmessage.info+=((QString)" ADS Contract Request Number = %1. Not fully implimented\n").arg((uchar)appmessage_bytes.at(i-1+2));
                i+=6;//goto next message
                break;
            }
            case Cancel_Emergency_Mode:
            {
                arincmessage.info+="Cancel_Emergency_Mode\n";
                i+=1;//goto next message
                break;
            }
            case Airframe_Identification_Group://not implimented. can't be bothered its just the AES # and we already got that
            {
                if((i+4)>appmessage_bytes.size()){fail=true;break;}//sz check
                arincmessage.info+="Airframe_Identification. Not implimented\n";
                i+=4;//goto next message
                break;
            }
            case Intermediate_Projected_Intent_Group:
            {
                if((i+9)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Intermediate_Projected_Intent_Group";

                //Distance
                double distance=((double)extractqint32(appmessage_bytes,i-1+3,0,16,false))*distance_scaller;
                //True track
                bool truetrack_isvalid=false;
                if(((appmessage_bytes.at(i-1+4)>>7)&0x01)==0)truetrack_isvalid=true;
                double truetrack=((double)extractqint32(appmessage_bytes,i-1+5,3,12,true))*truetrack_scaller;//i think doc had an error with this so have done it the way I thought was correct
                if(truetrack<0)truetrack+=360.0;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+7,3,16,true))*alt_scaller;
                //Projected Time
                double projected_time=((double)extractqint32(appmessage_bytes,i-1+9,5,14,false));

                if(truetrack_isvalid)arincmessage.info+=middlespacer+((QString)"Intermediate intent: Distance = %1 nm. True Track = %2 deg. Alt = %3 feet. Projected Time = %4\n").arg(distance).arg(qRound(truetrack)).arg(altitude).arg(QDateTime::fromTime_t(projected_time).toUTC().toString("hh:mm:ss"));
                 else middlespacer+((QString)"Intermediate intent: Distance = %1 nm. Alt = %2 feet. Projected Time = %3\n").arg(distance).arg(altitude).arg(QDateTime::fromTime_t(projected_time).toUTC().toString("hh:mm:ss"));

                i+=9;//goto next message
                break;
            }
            case Fixed_Projected_Intent_Group:
            {
                if((i+10)>appmessage_bytes.size()){fail=true;break;}//sz check
                //arincmessage.info+="Fixed_Projected_Intent_Group";

                //Lat
                double latitude=((double)extractqint32(appmessage_bytes,i-1+4,3,21,true))*lat_scaller;
                //Long
                double longitude=((double)extractqint32(appmessage_bytes,i-1+7,6,21,true))*long_scaller;
                //Alt
                double altitude=((double)extractqint32(appmessage_bytes,i-1+9,6,16,true))*alt_scaller;
                //Projected Time
                double projected_time=((double)extractqint32(appmessage_bytes,i-1+10,0,14,false));

                arincmessage.info+=middlespacer+((QString)"Fixed intent: Lat = %1 Long = %2 Alt = %3 feet. Projected Time = %4\n").arg(latitude).arg(longitude).arg(altitude).arg(QDateTime::fromTime_t(projected_time).toUTC().toString("hh:mm:ss"));

                i+=10;//goto next message
                break;
            }
            default:
                arincmessage.info+=((QString)"Group %1 unknown. Can't continue\n").arg((uchar)appmessage_bytes.at(i));
                fail=true;//fail cos we have no idea what to do

                /* not sure what would be best. hmmm
                //lets say we dont trust this packet. In future if they change the format we will have to either add the group or remove this
                downlinkgroups.clear();
                arincmessage.clear();
                arincmessage.downlink=acarsitem.downlink;
                */


                break;
            }
        }
        break;
    }
    default:
    // Not an ADS-C message - try other ACARS applications with libacars
        try_acars_apps(acarsitem, LA_MSG_DIR_AIR2GND);
        return true;
    }

    if(fail)arincmessage.info+="...\n";//"Failed understanding\n";//no need to bog the user down with to much detail

    //qDebug()<<" ";
    //qDebug()<<arincmessage.info.toLatin1().data();


    //if valid send a report to anyone who cares
    if(downlinkgroups.isValid())
    {

        //tmp time check. time check now done in sbs
       /* double ts_min=qFloor(downlinkgroups.adownlinkbasicreportgroup.time_stamp/60.0);
        double ts_sec=qFloor(downlinkgroups.adownlinkbasicreportgroup.time_stamp-ts_min*60.0);
        double ts_ms=qFloor((downlinkgroups.adownlinkbasicreportgroup.time_stamp-ts_min*60.0-ts_sec)*1000.0);
        QDateTime now=QDateTime::currentDateTimeUtc();
        QDateTime ts=now;
        ts.setTime(QTime(now.time().hour(),ts_min,ts_sec,ts_ms));
        if(ts.secsTo(now)<-1800)ts=ts.addSecs(-3600);
        if(ts.secsTo(now)>1800)ts=ts.addSecs(3600);
        if(qAbs(ts.secsTo(now))>900)//15mins
        {
            qDebug()<<"Time way out.";
            qDebug()<<acarsitem.message;
        }*/


        emit DownlinkGroupsSignal(downlinkgroups);


    }

    return true;

}
