#include "aerol.h"
#include <QtEndian>

//R channel

int RISUData::findisuitem(RISUItem &anisuitem)
{
    if(anisuitem.SUTYPE>11)return -1;
    if(anisuitem.SUTYPE<1)return -1;
    for(int i=0;i<isuitems.size();i++)
    {
        if((anisuitem.GESID==isuitems[i].GESID)&&(anisuitem.AESID==isuitems[i].AESID)&&(anisuitem.QNO==isuitems[i].QNO)&&(anisuitem.REFNO==isuitems[i].REFNO))return i;
    }
    return -1;
}
void RISUData::deleteoldisuitems()
{
    for(int i=0;i<isuitems.size();i++)
    {
        isuitems[i].count++;
        if(isuitems[i].count>10)//keep last 10 SUs
        {
            isuitems.removeAt(i);
            i--;
        }
    }
}
bool RISUData::update(QByteArray data)
{
    deleteoldisuitems();//limit number of isu saved

    int byte1=((uchar)data[-1+1]);
    int byte2=((uchar)data[-1+2]);
    int byte3=((uchar)data[-1+3]);
    int byte4=((uchar)data[-1+4]);
    int byte5=((uchar)data[-1+5]);
    int byte6=((uchar)data[-1+6]);

    anisuitem.clear();
    anisuitem.SEQINDICATOR=((byte1&0xF0)>>4);
    anisuitem.SUTYPE=byte1&0x0F;
    anisuitem.QNO=((byte2&0xF0)>>4);
    anisuitem.REFNO=byte2&0x07;
    anisuitem.AESID=byte3<<8*2|byte4<<8*1|byte5<<8*0;
    anisuitem.GESID=byte6;

    int idx=findisuitem(anisuitem);
    if(idx<0)
    {
        isuitems.push_back(anisuitem);
        idx=isuitems.size()-1;
    }
    RISUItem *pisuitem;
    pisuitem=&isuitems[idx];
    pisuitem->count=0;

    int SUTotal=0;
    int SUindex=0;
    switch(anisuitem.SEQINDICATOR)
    {
    case 1:
        SUTotal=1;
        SUindex=0;
        break;
    case 2:
        SUTotal=2;
        SUindex=0;
        break;
    case 3:
        SUTotal=2;
        SUindex=1;
        break;
    case 4:
        SUTotal=3;
        SUindex=0;
        break;
    case 5:
        SUTotal=3;
        SUindex=1;
        break;
    case 6:
        SUTotal=3;
        SUindex=2;
        break;
    default:
        break;
    }
    int BytesInSU=0;
    if((anisuitem.SUTYPE>=1)&&(anisuitem.SUTYPE<=11))BytesInSU=anisuitem.SUTYPE;
    bool SignalingInfoSU=false;
    if(anisuitem.SUTYPE==15)SignalingInfoSU=true;

    int thisnum=11*SUTotal-11+BytesInSU;
    if(thisnum>0)
    {
        if(pisuitem->userdata.size()==0)pisuitem->userdata.resize(thisnum);
        if(thisnum<pisuitem->userdata.size())pisuitem->userdata.resize(thisnum);
    }
    if(!SignalingInfoSU)
    {
        for(int i=0-1+7;i<BytesInSU-1+7;i++)pisuitem->userdata[i+11*SUindex+1-7]=data[i];
        pisuitem->filledarray|=(1<<SUindex);
    } else pisuitem->userdata.clear();

    if((SignalingInfoSU)||((pisuitem->filledarray==7)&&(SUTotal==3))||((pisuitem->filledarray==3)&&(SUTotal==2))||((pisuitem->filledarray==1)&&(SUTotal==1)))
    {
        lastvalidisuitem=*pisuitem;
        isuitems.removeAt(idx);
        return true;
    }

    return false;
}

//

int ISUData::findisuitem71(ISUItem &anisuitem)
{
    if(anisuitem.NOOCTLESTINLASTSSU>8)return -1;
    for(int i=0;i<isuitems.size();i++)
    {
        if((anisuitem.AESID==isuitems[i].AESID)&&(anisuitem.GESID==isuitems[i].GESID)&&(anisuitem.QNO==isuitems[i].QNO)&&(anisuitem.REFNO==isuitems[i].REFNO))return i;
    }
    return -1;
}
int ISUData::findisuitemC0(ISUItem &anisuitem)
{
    if(anisuitem.NOOCTLESTINLASTSSU>8)return -1;
    for(int i=0;i<isuitems.size();i++)
    {
        if(((anisuitem.AESID==isuitems[i].AESID)
            &&(anisuitem.GESID==isuitems[i].GESID)
            &&(anisuitem.SEQNO+1)==isuitems[i].SEQNO)
            &&(anisuitem.QNO==isuitems[i].QNO)
            &&(anisuitem.REFNO==isuitems[i].REFNO))
            return i;
    }
    return -1;
}
void ISUData::deleteoldisuitems()
{
    for(int i=0;i<isuitems.size();i++)
    {
        isuitems[i].count++;
        if(isuitems[i].count>10)//keep last 10 0x71 SUs
        {
            isuitems.removeAt(i);
            i--;
        }
    }
}
bool ISUData::update(QByteArray data)
{
    missingssu=false;
    assert(data.size()>=10);
    uchar message=data[0];
    switch(message)
    {
    case AEROTypeP::User_data_ISU_RLS_P_T_channel:
    {

        deleteoldisuitems();//limit number of isu saved

        anisuitem.AESID=((uchar)data[1])<<8*2|((uchar)data[2])<<8*1|((uchar)data[3])<<8*0;
        anisuitem.GESID=(uchar)data[4];
        uchar val=data[5];
        anisuitem.QNO=(val>>4)&0x0F;
        anisuitem.REFNO=val&0x0F;
        val=data[6];
        anisuitem.SEQNO=val&0x3F;
        val=data[7];
        anisuitem.NOOCTLESTINLASTSSU=(val>>4)&0x0F;
        anisuitem.count=0;
        anisuitem.userdata.clear();
        for(int i=8;i<=9;i++)anisuitem.userdata+=data[i];

        int idx=findisuitem71(anisuitem);
        if(idx<0)isuitems.push_back(anisuitem);
        else isuitems[idx]=anisuitem;

        //qDebug()<<(((QString)"71 isudata: AESID = %1 GESID = %2 QNO = %3 REFNO = %4 SEQNO = %5 NOOCTLESTINLASTSSU = %6").arg(anisuitem.AESID).arg(anisuitem.GESID).arg(anisuitem.QNO).arg(anisuitem.REFNO).arg(anisuitem.SEQNO).arg(anisuitem.NOOCTLESTINLASTSSU));
    }
        break;
    default:
    {
        if((message&0xC0)!=0xC0)break;
        anisuitem.SEQNO=message&0x3F;
        int val=data[1];
        anisuitem.QNO=(val>>4)&0x0F;
        anisuitem.REFNO=val&0x0F;

        int idx=findisuitemC0(anisuitem);
        if(idx<0)
        {

            //qDebug()<<"missing su/ssu";
            missingssu=true;
            return false;
        }
        ISUItem *pisuitem;
        pisuitem=&isuitems[idx];

        (pisuitem->SEQNO)--;
        if(pisuitem->SEQNO==0)
        {
            for(int i=2;i<=(pisuitem->NOOCTLESTINLASTSSU+1);i++)pisuitem->userdata+=data[i];
            lastvalidisuitem=*pisuitem;
            //qDebug()<<"final ssu";
            return true;
        }
        else for(int i=2;i<=9;i++)pisuitem->userdata+=data[i];

        //qDebug()<<((QString)"").sprintf("ssu data: SEQNO %d, QNO %d, REFNO %d",pisuitem->SEQNO,pisuitem->QNO,pisuitem->REFNO);
    }
        break;
    }

    return false;
}

int ACARSDefragmenter::findfragment(ACARSItem &acarsitem)
{
    //    QString tmp;
    for(int idx=0;idx<acarsitemexts.size();idx++)
    {
        ACARSItemext *pitem=&acarsitemexts[idx];
        /*if(acarsitem.isuitem.AESID==pitem->anacarsitem.isuitem.AESID)
        {
            qDebug()<<"same AESID";
            qDebug()<<acarsitem.PLANEREG<<pitem->anacarsitem.PLANEREG;
            qDebug()<<acarsitem.LABEL<<pitem->anacarsitem.LABEL;
            qDebug()<<acarsitem.MODE<<pitem->anacarsitem.MODE;
            qDebug()<<acarsitem.TAK<<pitem->anacarsitem.TAK;
            qDebug()<<acarsitem.isuitem.AESID<<pitem->anacarsitem.isuitem.AESID;
            qDebug()<<acarsitem.isuitem.GESID<<pitem->anacarsitem.isuitem.GESID;
            qDebug()<<pitem->anacarsitem.moretocome;
            qDebug()<<acarsitem.isuitem.REFNO<<pitem->anacarsitem.isuitem.REFNO;
            qDebug()<<acarsitem.isuitem.SEQNO<<pitem->anacarsitem.isuitem.SEQNO;
        }*/
        if(
                (acarsitem.PLANEREG==pitem->anacarsitem.PLANEREG)&&
                (acarsitem.LABEL==pitem->anacarsitem.LABEL)&&
                (acarsitem.MODE==pitem->anacarsitem.MODE)&&
                // (acarsitem.TAK==pitem->anacarsitem.TAK)&& //i found once time TAK did not match should we skip this check??
                (acarsitem.isuitem.AESID==pitem->anacarsitem.isuitem.AESID)&&
                (acarsitem.isuitem.GESID==pitem->anacarsitem.isuitem.GESID)&&
                (pitem->anacarsitem.moretocome)
                )
        {

            if(acarsitem.TAK!=pitem->anacarsitem.TAK)
            {
                //        qDebug()<<"TAK is different??";
                continue;
            }

            //            qDebug()<<(char)pitem->anacarsitem.BI;
            //            qDebug()<<(char)acarsitem.BI;

            uchar expnewbi=(((pitem->anacarsitem.BI+1)-'A')%26)+'A';
            if(expnewbi==acarsitem.BI)
            {
                //                qDebug()<<"found acars fragment"<<idx;
                return idx;
            }
            else
            {
                //         tmp="failed count test.";
                /*+expnewbi+acarsitem.BI;
                tmp+=acarsitem.PLANEREG+"\n";
                tmp+=acarsitem.LABEL+"\n";
                tmp+=acarsitem.isuitem.AESID+"\n";
                tmp+=pitem->anacarsitem.moretocome+"\n";
                tmp+=acarsitem.isuitem.REFNO+"\n";*/

            }

        }
    }
    //if(!tmp.isEmpty())qDebug()<<tmp;
    return -1;
}
bool ACARSDefragmenter::defragment(ACARSItem &acarsitem)
{

    //flush old frags
    for(int i=0;i<acarsitemexts.size();i++)
    {
        acarsitemexts[i].count++;
        if(acarsitemexts[i].count>30)
        {
            acarsitemexts.removeAt(i);
            i--;
        }
    }

    //if cant find frag then do nothing else push a new frag
    int idx=findfragment(acarsitem);
    if(idx<0)
    {
        if(!acarsitem.moretocome)return true;
        anacarsitemext.anacarsitem=acarsitem;
        anacarsitemext.count=0;
        acarsitemexts.push_back(anacarsitemext);
        //qDebug()<<"pushed first acars fragment"<<anacarsitemext.anacarsitem.isuitem.AESID;
        return false;
    }

    //qDebug()<<"adding acars fragment"<<idx;

    //update frag
    ACARSItemext *polditem=&acarsitemexts[idx];
    polditem->count=0;
    polditem->anacarsitem.BI=acarsitem.BI;
    polditem->anacarsitem.message+=acarsitem.message;
    polditem->anacarsitem.moretocome=acarsitem.moretocome;

    //more to come then do nothing
    if(acarsitem.moretocome)return false;

    //qDebug()<<"last acars fragment"<<idx;

    //else return defragmented item with BI equal to last acars message
    acarsitem=polditem->anacarsitem;
    acarsitemexts.removeAt(idx);
    return true;


}

ParserISU::ParserISU(QObject *parent):
    QObject(parent)
{
    downlink=false;
    //dblookup
    dbtu = new DataBaseTextUser(this);
    connect(dbtu,SIGNAL(result(bool,int,QStringList)),this,SLOT(acarslookupresult(bool,int,QStringList)));

}
bool ParserISU::parse(ISUItem &isuitem)
{

    if(isuitem.AESID==0)
    {
        anerror="Error: AESID == 0";
        emit Errorsignal(anerror);

        return false;
    }
    QVector<int> parities;
    QByteArray textish;
    for(int i=0;i<isuitem.userdata.size();i++)
    {
        int byte=(uchar)isuitem.userdata[i];
        int parity=0;for(int j=0;j<8;j++)if((byte>>j)&0x01)parity++;
        parity&=1;
        if(parity)parities.push_back(0x01);
        else parities.push_back(0x00);
        byte&=0x7F;
        textish+=byte;
    }

    //fill acars message in

    //check that it matches the pattern of acars message
    //(this is 8bit parity makes things differnet)
    //FF FF *
    //01 (SOH) *
    //32 (mode)
    //AE C8 C2 AD 4A C8 CD (reg)
    //15 C1 B6 51 (??? seem to have parity but not printable, i think this is always 4 bytes)
    //02 (STX) (only if text is there)
    //2F D0 49 CB 43 D0 D9 C1 AE C1 C4 D3 AE C8 C2 AD 4A C8 CD B0 37 B0 34 B0 C2 B0 B0 B0 43 B0 B0 B0 C4 B0 31 B0 45 B0 31 31 B0 B0 B0 34 34 B0 C4
    //83 or 97 (ETX/ETB) * (ETB when text is over 220 chars)
    //93 AB (BSC no parity bits)
    //7F (DEL) *

    /*QString str;
    for(int i=0;i<isuitem.userdata.size();i++)str+=(((QString)"%1").arg((uchar)isuitem.userdata[i],2, 16, QChar('0'))).toUpper()   ;
    //qDebug()<<"yes"<<risudata.lastvalidisuitem.userdata;
    qDebug()<<str;*/

    bool isacars = false;

    // not sure about some of these conditions, certainly missing some messages with all of them on
    if((isuitem.userdata.size()>16)
            &&((uchar)isuitem.userdata[0])==0xFF
            &&((uchar)isuitem.userdata[1])==0xFF
           // &&((uchar)isuitem.userdata[2])==0x01

            //&&((((uchar)isuitem.userdata[isuitem.userdata.size()-1-3])==0x83)
            //   ||(((uchar)isuitem.userdata[isuitem.userdata.size()-1-3])==0x97))

            //&&((uchar)isuitem.userdata[isuitem.userdata.size()-1])==0x7F
            &&(((uchar)isuitem.userdata[15])==0x83 || ((uchar)isuitem.userdata[15])==0x02 ))
        isacars = true;

    if(isacars)
    {


        //fill in header info
        anacarsitem.clear();
        anacarsitem.downlink=downlink;
        anacarsitem.isuitem=isuitem;
        uchar byte=((uchar)isuitem.userdata[3]);
        anacarsitem.MODE=byte&0x7F;
        anacarsitem.TAK=((uchar)textish[11]);
        anacarsitem.LABEL+=((uchar)textish[12]);
        anacarsitem.LABEL+=((uchar)textish[13]);
        anacarsitem.BI=((uchar)textish[14]);
        if(((uchar)isuitem.userdata[15])==0x02)anacarsitem.hastext=true;
        if(((uchar)isuitem.userdata[isuitem.userdata.size()-1-3])==0x97){
            anacarsitem.moretocome=true;
        }
        for(int k=4;k<4+7;k++)
        {
            byte=((uchar)isuitem.userdata[k]);
            byte&=0x7F;
            if(!parities[k])
            {
               anerror=((QString)"").sprintf("ISU: AESID = %X GESID = %X QNO = %02X REFNO = %02X : Parity error",isuitem.AESID,isuitem.GESID,isuitem.QNO,isuitem.REFNO);
               emit Errorsignal(anerror);
               return false;
            }
            anacarsitem.PLANEREG+=(char)byte;


        }


        //fill in message
        if(anacarsitem.hastext)
        {
            for(int k=16;k<isuitem.userdata.size()-1-3;k++)
            {
                byte=((uchar)isuitem.userdata[k]);
                byte&=0x7F;
                if(!parities[k])
                {
                    anerror=((QString)"").sprintf("ISU: AESID = %X GESID = %X QNO = %02X REFNO = %02X : Parity error",isuitem.AESID,isuitem.GESID,isuitem.QNO,isuitem.REFNO);
                    emit Errorsignal(anerror);

                   return false;
                }

                if((byte<0x20)&&(!(byte==10||byte==13)))
                {
                    //                    qDebug()<<"This is strange. byte<0x20 byte="<<byte;
                }
                if(byte==0x7F)anacarsitem.message+="<DEL>";
                else anacarsitem.message+=(char)byte;

            }
        }

        //mark as valid
        anacarsitem.valid=true;

       //send acars message to lookup if fully defraged
        if(acarsdefragmenter.defragment(anacarsitem))
        {
            ACARSItem *pai=new ACARSItem;
            *pai=anacarsitem;
            QString AESIDstr=((QString)"").sprintf("%06X",anacarsitem.isuitem.AESID);
            dbtu->request(databasedir,AESIDstr,pai);
        }

        return true;
    }

    //not acars message so say so and return the plain hex data as the message
    anacarsitem.clear();
    anacarsitem.downlink=downlink;
    anacarsitem.isuitem=isuitem;
    anacarsitem.message.clear();
    anacarsitem.nonacars=true;
    for(int i=0;i<isuitem.userdata.size();i++)anacarsitem.message+=(((QString)"%1").arg((uchar)isuitem.userdata[i],2, 16, QChar('0'))).toUpper();
    anacarsitem.valid=true;

    ACARSItem *pai=new ACARSItem;
    *pai=anacarsitem;
    QString AESIDstr=((QString)"").sprintf("%06X",anacarsitem.isuitem.AESID);
    dbtu->request(databasedir,AESIDstr,pai);

    return true;

}
void ParserISU::setDataBaseDir(const QString &dir)
{
    databasedir=dir;
}
void ParserISU::acarslookupresult(bool ok, int ref, const QStringList &result)
{
    Q_UNUSED(ok);
    ACARSItem *panacarsitem=((ACARSItem*)dbtu->getuserdata(ref));
    if(panacarsitem==NULL)return;

    //remove prepending dots
    int i=0;while((i<panacarsitem->PLANEREG.size())&&(panacarsitem->PLANEREG[i]=='.'))i++;
    panacarsitem->PLANEREG=panacarsitem->PLANEREG.right(panacarsitem->PLANEREG.size()-i);

    if(result.size()==QMetaEnum::fromType<DataBaseTextUser::DataBaseSchema>().keyCount())
    {
        //if non acars message then fill in reg with lookup value
        if(panacarsitem->nonacars)
        {
            panacarsitem->PLANEREG=result[DataBaseTextUser::DataBaseSchema::Registration].toLatin1();
        }
        //if sat reg and lookup reg are very different then we have a problem
        //do we use the db reg or the reg sent by the sv?????
        //lets just say if panacarsitem->PLANEREG is empty then use the DB
        if(panacarsitem->PLANEREG.trimmed().isEmpty())
        {
            panacarsitem->PLANEREG=result[DataBaseTextUser::DataBaseSchema::Registration].toLatin1();
        }
    }
    panacarsitem->dblookupresult=result;
    emit ACARSsignal(*panacarsitem);
    delete panacarsitem;
}

AeroLInterleaver::AeroLInterleaver()
{
    M=64;

    interleaverowpermute.resize(M);
    interleaverowdepermute.resize(M);

    //this is 1-1 and onto
    for(int i=0;i<M;i++)
    {
        interleaverowpermute[(i*27)%M]=i;
        interleaverowdepermute[i]=(i*27)%M;
        //        interleaverowdepermute[(i*19)%M]=i;
    }
    setSize(6);
}
void AeroLInterleaver::setSize(int _N)
{
    if(_N<1)return;
    N=_N;
    matrix.resize(M*N);
    matrix_ba.resize(M*N);
    for(int a = 0; a < matrix_ba.length(); a++)
    {
        matrix_ba[a] = 0;
    }
}
QVector<int> &AeroLInterleaver::interleave(QVector<int> &block)
{
    assert(block.size()==(M*N));
    int k=0;
    for(int i=0;i<M;i++)
    {
        for(int j=0;j<N;j++)
        {
            int entry=interleaverowpermute[i]+M*j;
            assert(entry<block.size());
            assert(k<matrix.size());
            matrix[k]=block[entry];
            k++;
        }
    }
    return matrix;
}
QVector<int> &AeroLInterleaver::deinterleave(QVector<int> &block)
{
    assert(block.size()==(M*N));
    int k=0;
    for(int j=0;j<N;j++)
    {
        for(int i=0;i<M;i++)
        {
            int entry=interleaverowdepermute[i]*N+j;
            assert(entry<block.size());
            assert(k<matrix.size());
            matrix[k]=block[entry];
            k++;
        }
    }
    return matrix;
}
QVector<int> &AeroLInterleaver::deinterleave(QVector<int> &block,int cols)
{
    assert(cols<=N);
    assert(block.size()>=(M*cols));
    int k=0;
    for(int j=0;j<cols;j++)
    {
        for(int i=0;i<M;i++)
        {
            int entry=interleaverowdepermute[i]*cols+j;
            assert(entry<block.size());
            assert(k<matrix.size());
            matrix[k]=block[entry];
            k++;
        }
    }
    return matrix;
}

QByteArray &AeroLInterleaver::deinterleave_ba(QVector<int> &block,int cols)
{

    // default to MSK settings if zero
    if(cols==0){
        cols = N;
    }
    assert(cols<=N);
    assert(block.size()>=(M*cols));
    int k=0;
    for(int j=0;j<cols;j++)
    {
        for(int i=0;i<M;i++)
        {
            int entry=interleaverowdepermute[i]*cols+j;
            assert(entry<block.size());
            assert(k<matrix_ba.size());
            matrix_ba[k]=block[entry];
            k++;
        }
    }
    return matrix_ba;
}


QVector<int> &AeroLInterleaver::deinterleaveMSK(QVector<int> &block, int blocks)
{
    assert(block.size()>=(M*blocks));

    // we need to first deinterleave 5 cols for the first block then 3 cols for the remaining blocks
    int k=0;

    for(int j=0;j<5;j++)
    {
        for(int i=0;i<M;i++)
        {
            int entry=interleaverowdepermute[i]*5+j;
            assert(entry<5*M);
            assert(k<matrix.size());
            matrix[k]=block[entry];
            k++;
        }

    }

    int procblocks = 5;
    while (k < blocks *M){


        for(int j=0;j<3;j++)
        {
            for(int i=0;i<M;i++)
            {
                int entry=(M*procblocks) + (interleaverowdepermute[i]*3+j);
                assert(entry < block.size());
                assert(k<matrix.size());
                matrix[k]=block[entry];
                k++;

            }
        }
        procblocks+=3;
    }

    return matrix;
}


QByteArray &AeroLInterleaver::deinterleaveMSK_ba(QVector<int> &block, int blocks)
{
    assert(block.size()>=(M*blocks));

    // we need to first deinterleave 5 cols for the first block then 3 cols for the remaining blocks
    int k=0;

    for(int j=0;j<5;j++)
    {
        for(int i=0;i<M;i++)
        {
            int entry=interleaverowdepermute[i]*5+j;
            assert(entry<5*M);
            assert(k<matrix_ba.size());
            matrix_ba[k]=(char)block[entry];
            k++;
        }

    }


    int procblocks = 5;

    while (k < blocks *M){


        for(int j=0;j<3;j++)
        {
            for(int i=0;i<M;i++)
            {
                int entry=(M*procblocks) + (interleaverowdepermute[i]*3+j);
                assert(entry < block.size());
                assert(k<matrix_ba.size());
                matrix_ba[k]=(char)block[entry];
                k++;

            }
        }
        procblocks+=3;
    }


    return matrix_ba;
}


PreambleDetector::PreambleDetector()
{
    preamble.resize(1);
    buffer.resize(1);
    buffer_ptr=0;
}
void PreambleDetector::setPreamble(QVector<int> _preamble)
{
    preamble=_preamble;
    if(preamble.size()<1)preamble.resize(1);
    buffer.fill(0,preamble.size());
    buffer_ptr=0;
}
bool PreambleDetector::setPreamble(quint64 bitpreamble,int len)
{
    if(len<1||len>64)return false;
    preamble.clear();
    for(int i=len-1;i>=0;i--)
    {
        if((bitpreamble>>i)&1)preamble.push_back(1);
        else preamble.push_back(0);
    }
    if(preamble.size()<1)preamble.resize(1);
    buffer.fill(0,preamble.size());
    buffer_ptr=0;
    return true;
}
bool PreambleDetector::Update(int val)
{ 
    for(int i=0;i<(buffer.size()-1);i++)buffer[i]=buffer[i+1];
    buffer[buffer.size()-1]=val;
    if(buffer==preamble){buffer.fill(0);return true;}
    return false;
}

PreambleDetectorPhaseInvariant::PreambleDetectorPhaseInvariant()
{
    inverted=false;
    preamble.resize(1);
    buffer.resize(1);
    buffer_ptr=0;
    tollerence=0;
}
void PreambleDetectorPhaseInvariant::setPreamble(QVector<int> _preamble)
{
    preamble=_preamble;
    if(preamble.size()<1)preamble.resize(1);
    buffer.fill(0,preamble.size());
    buffer_ptr=0;
}
bool PreambleDetectorPhaseInvariant::setPreamble(quint64 bitpreamble,int len)
{
    if(len<1||len>64)return false;
    preamble.clear();
    for(int i=len-1;i>=0;i--)
    {
        if((bitpreamble>>i)&1)preamble.push_back(1);
        else preamble.push_back(0);
    }
    if(preamble.size()<1)preamble.resize(1);
    buffer.fill(0,preamble.size());
    buffer_ptr=0;
    return true;
}
int PreambleDetectorPhaseInvariant::Update(int val)
{
    assert(buffer.size()==preamble.size());
    int xorsum=0;
    for(int i=0;i<(buffer.size()-1);i++)
    {
        buffer[i]= buffer.at(i+1);
        xorsum+=buffer.at(i)^preamble.at(i);
    }
    xorsum+=val^preamble.at(buffer.size()-1);
    buffer[buffer.size()-1]=val;
    if(xorsum>=(buffer.size()-tollerence)){


        inverted=true;
        return true;
    }
    if(xorsum<=tollerence){
        inverted=false;

        return true;
    }
    return false;
}
void PreambleDetectorPhaseInvariant::setTollerence(int _tollerence)
{
    tollerence=_tollerence;
}

/* for C Channels*/
OQPSKPreambleDetectorAndAmbiguityCorrection::OQPSKPreambleDetectorAndAmbiguityCorrection()
{
    inverted=false;
    preamble1.resize(1);
    buffer1.resize(1);
    preamble2.resize(1);
    buffer2.resize(1);

    buffer_ptr=0;
    tollerence=0;
}

bool OQPSKPreambleDetectorAndAmbiguityCorrection::setPreamble(quint64 bitpreamble1,quint64 bitpreamble2,int len)
{
    if(len<1||len>64)return false;
    preamble1.clear();
    preamble2.clear();

    for(int i=len-1;i>=0;i--)
    {
        if((bitpreamble1>>i)&1)preamble1.push_back(1);
        else preamble1.push_back(0);

        if((bitpreamble2>>i)&1)preamble2.push_back(1);
        else preamble2.push_back(0);
    }
    if(preamble1.size()<1)preamble1.resize(1);
    buffer1.fill(0,preamble1.size());

    if(preamble2.size()<1)preamble2.resize(1);
    buffer2.fill(0,preamble2.size());

    buffer_ptr=0;


    return true;
}
int OQPSKPreambleDetectorAndAmbiguityCorrection::Update(int val)
{
    assert(buffer1.size()==preamble1.size());
    assert(buffer2.size()==preamble2.size());

    /* check the first buffer and preamble */
    int xorsum=0;
    for(int i=0;i<(buffer1.size()-1);i++)
    {
        buffer1[i]=buffer1[i+1];
        xorsum+=buffer1[i]^preamble1[i];
    }
    xorsum+=val^preamble1[buffer1.size()-1];
    buffer1[buffer1.size()-1]=val;
    if(xorsum>=(buffer1.size()-tollerence)){

        inverted=true;

        return true;
    }
    if(xorsum<=tollerence){

        inverted=false;

        return true;
    }

    xorsum=0;
    for(int i=0;i<(buffer2.size()-1);i++)
    {
        buffer2[i]=buffer2[i+1];
        xorsum+=buffer2[i]^preamble2[i];
    }
    xorsum+=val^preamble2[buffer2.size()-1];
    buffer2[buffer2.size()-1]=val;
    if(xorsum>=(buffer2.size()-tollerence)){

        inverted=true;
        return true;
    }
    if(xorsum<=tollerence){

        inverted=false;
        return true;
    }


   return false;
}
void OQPSKPreambleDetectorAndAmbiguityCorrection::setTollerence(int _tollerence)
{
    tollerence=_tollerence;
}



AeroL::AeroL(QObject *parent) : QIODevice(parent)
{

    cntr=1000000000;
    formatid=0;
    supfrmaker=0;
    framecounter1=0;
    framecounter2=0;
    gotsync_last=0;
    blockcnt=-1;

    //install parser
    parserisu = new ParserISU(this);
    connect(parserisu,SIGNAL(ACARSsignal(ACARSItem&)),this,SIGNAL(ACARSsignal(ACARSItem&)));
    connect(parserisu,SIGNAL(Errorsignal(QString&)),this,SIGNAL(Errorsignal(QString&)));


    donotdisplaysus.clear();

    cntr=1000000000;
    datacdcountdown=0;
    datacd=false;
    emit DataCarrierDetect(datacd);

    QTimer *dcdtimer=new QTimer(this);
    connect(dcdtimer,SIGNAL(timeout()),this,SLOT(updateDCD()));
    dcdtimer->start(1000);

    sbits.reserve(1000);
    decodedbytes.reserve(1000);

    //new viterbi decoder (this is a qobject and has us as perent so is deleted automatically)
    jconvolcodec = new JConvolutionalCodec(this);
    QVector<quint16> polys;
    polys.push_back(109);
    polys.push_back(79);
    jconvolcodec->SetCode(2,7,polys,24);



    dl1.setLength(12);//delay for decode encode BER check
    dl2.setLength(576-6);//delay's data to next frame

    preambledetector.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011

    //I	1010 1011 0011 0111 0110 1001 0011 1000 1011 1100 1010 0011 0000 = 0xAB376938BCA30 hex = 3012071630031408 decimal
    //Q	0000 1100 0101 0011 1101 0001 1100 1001 0110 1110 1100 1101 0101 = 0xC53D1C96ECD5 hex = 216866263330005 decimal

    // doual preamble detectors for C Channel
    preambledetectorreal.setPreamble(216866263330005LL,3012071630031408LL,52);
    preambledetectorimag.setPreamble(216866263330005LL,3012071630031408LL,52);

    index = 0;

    //Preamble detector for OQPSK
    preambledetectorphaseinvariantimag.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011
    preambledetectorphaseinvariantreal.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011

    // MSK burst preamble detector
    mskBurstDetector.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011

    //Preamble for start of burst
    QVector<int> pre;
    pre.push_back(0x11);
    pre.push_back(0x07);
    pre.push_back(0x42);
    pre.push_back(0x00);
    pre.push_back(0x00);
    pre.push_back(0x13);
    pre.push_back(0x09);
    preambledetectorburst.setPreamble(pre);

    setSettings(1200,false);

}

void AeroL::setBitRate(double fb)
{
    setSettings(fb,burstmode);
}

void AeroL::setBurstmode(bool _burstmode)
{
    setSettings(ifb,_burstmode);
}

void AeroL::setSettings(double fb, bool _burstmode)
{
    isudata.reset();
    risudata.reset();
    burstmode=_burstmode;

    if(burstmode)
    {
        preambledetectorphaseinvariantimag.setTollerence(4);
        preambledetectorphaseinvariantreal.setTollerence(4);
        preambledetectorimag.setTollerence(0);
        preambledetectorreal.setTollerence(0);
        mskBurstDetector.setTollerence(4);
    }
    else
    {
        preambledetectorphaseinvariantimag.setTollerence(0);
        preambledetectorphaseinvariantreal.setTollerence(0);
        preambledetectorimag.setTollerence(6);
        preambledetectorreal.setTollerence(6);
        mskBurstDetector.setTollerence(0);
    }
    ifb=qRound(fb);
    switch(ifb)
    {
    case 600:
        leaver.setSize(6);//9 for 1200 bps, 6 for 600 bps
        block.resize(6*64);
        dl2.setLength(576-6);//delay's data to next frame
        AERO_SPEC_NumberOfBits=1152;
        AERO_SPEC_BitsInHeader=16;
        AERO_SPEC_TotalNumberOfBits=AERO_SPEC_BitsInHeader+AERO_SPEC_NumberOfBits+32;
        useingOQPSK=false;
        break;
    case 1200:
        leaver.setSize(9);//9 for 1200 bps, 6 for 600 bps
        block.resize(9*64);
        dl2.setLength(576-6);//delay's data to next frame
        AERO_SPEC_NumberOfBits=1152;
        AERO_SPEC_BitsInHeader=16;
        AERO_SPEC_TotalNumberOfBits=AERO_SPEC_BitsInHeader+AERO_SPEC_NumberOfBits+32;
        useingOQPSK=false;
        break;
    case 8400:
        leaver.setSize(4);//4 rows at 64 bits each
        block.resize(4*64); // 1 deleaver block
        deleaveredBlock.resize(16*4*64);
        dl2.setLength(2714-6);//delay's data to next frame
        depuncturedBlock.reserve(5460);
        AERO_SPEC_NumberOfBits=4096;
        AERO_SPEC_BitsInHeader=0;//178 dummy bits
        AERO_SPEC_TotalNumberOfBits=AERO_SPEC_BitsInHeader+AERO_SPEC_NumberOfBits;
        useingOQPSK=true;
        break;
    case 10500:
        leaver.setSize(78);//78 for 10.5k
        block.resize(78*64);
        dl2.setLength(4992-6);//delay's data to next frame
        AERO_SPEC_NumberOfBits=4992;
        AERO_SPEC_BitsInHeader=16+178;//178 dummy bits
        AERO_SPEC_TotalNumberOfBits=AERO_SPEC_BitsInHeader+AERO_SPEC_NumberOfBits+64;
        useingOQPSK=true;
        break;
    default:
        leaver.setSize(9);//9 for 1200 bps, 6 for 600 bps
        block.resize(9*64);
        AERO_SPEC_NumberOfBits=1152;
        AERO_SPEC_BitsInHeader=16;
        useingOQPSK=false;
        break;
    }

    if(burstmode){

        if(useingOQPSK){
            AERO_SPEC_TotalNumberOfBits=ifb;//1 sec countdown for burst modes
        }
        else{
            AERO_SPEC_TotalNumberOfBits=ifb*3;//3 sec countdown for burst modes
        }
    }

}

AeroL::~AeroL()
{

}

bool AeroL::Start()
{
    if(psinkdevice.isNull())return false;
    QIODevice *out=psinkdevice.data();
    out->open(QIODevice::WriteOnly);
    return true;
}

void AeroL::Stop()
{
    if(!psinkdevice.isNull())
    {
        QIODevice *out=psinkdevice.data();
        out->close();
    }
}

void AeroL::ConnectSinkDevice(QIODevice *device)
{
    if(!device)return;
    psinkdevice=device;
    Start();
}

void AeroL::DisconnectSinkDevice()
{
    Stop();
    if(!psinkdevice.isNull())psinkdevice.clear();
}

void AeroL::updateDCD()
{
    //qDebug()<<datacdcountdown;

    //keep track of the DCD
    if(datacdcountdown>0)datacdcountdown-=3;
    else {if(datacdcountdown<0)datacdcountdown=0;}
    if(datacd&&!datacdcountdown)
    {
        datacd=false;

        emit DataCarrierDetect(datacd);
    }
}

QByteArray &AeroL::Decode(QVector<short> &bits, bool soft)//0 bit --> oldest bit
{
    decodedbytes.clear();

    quint16 bit=0;
    quint16 soft_bit=0;

    for(int i=0;i<bits.size();i++)
    {

        if(soft)
        {
            if(((uchar)bits[i])>=128)bit=1;
            else bit=0;
        }
         else
         {
            bit=bits[i];
         }
        soft_bit=bits[i];

        //for burst mode to allow tolerance of UW
        if(bits[i]<0)
        {
            //qDebug()<<"start of packet";
            muw=0;
            continue;
        }
        if(muw<100000)muw++;

        //Preamble detector and ambiguity corrector for OQPSK
        int gotsync;
        if(useingOQPSK)
        {
            realimag++;realimag%=2;
            if(realimag)
            {

                if(cntr > AERO_SPEC_NumberOfBits-68 || cntr <= 0 ||!datacd)
                {
                gotsync=preambledetectorphaseinvariantimag.Update(bit);
                if(!gotsync_last)
                {
                    gotsync_last=gotsync;
                    gotsync=0;
                } else gotsync_last=0;
                }else{
                    gotsync = false;
                    gotsync_last = false;
                }
            }
             else
             {
                if(cntr > AERO_SPEC_NumberOfBits-68 || cntr <= 0 ||!datacd)
                {
                gotsync=preambledetectorphaseinvariantreal.Update(bit);
                if(!gotsync_last)
                {
                    gotsync_last=gotsync;
                    gotsync=0;
                } else gotsync_last=0;

                }else{
                    gotsync = false;
                    gotsync_last = false;
                }
             }

            //for 10500 UW should be about 80 samples after start of packet signal from demodulator if not we have a false positive
            if(gotsync&&burstmode)
            {
                if(ifb==10500&&(abs(muw-80)>150))
                {
                    //qDebug()<<"wrong timing";
                    gotsync=false;
                }
            }

            if(realimag)
            {
                if(preambledetectorphaseinvariantimag.inverted)
                {
                    bit=1-bit;

                    if(soft_bit > 128){
                         soft_bit = 255-soft_bit;
                    } else if (soft_bit < 128){
                        soft_bit = 255-soft_bit;
                    }


                }

            }
            else
            {
                if(preambledetectorphaseinvariantreal.inverted)
                {
                    bit=1-bit;

                    if(soft_bit > 128)
                    {
                         soft_bit = 255-soft_bit;
                    }
                     else if (soft_bit < 128)
                     {
                        soft_bit = 255-soft_bit;
                     }
                }
            }

        } //non 10500 burst mode use a phase invariant preamble detector
         else if(burstmode)
         {

            bool inverted = mskBurstDetector.inverted;

            gotsync=mskBurstDetector.Update(bit);

            if( muw > 250 && gotsync)
            {

                if(inverted != mskBurstDetector.inverted)
                {
                    mskBurstDetector.inverted = inverted;
                }
                gotsync = false;

            }

            if(mskBurstDetector.inverted)
            {

                bit=1-bit;

                if(soft_bit > 128)
                {
                     soft_bit = 255-soft_bit;
                }
                 else if (soft_bit < 128)
                 {
                    soft_bit = 255-soft_bit;
                 }
            }
         }
          else
          {
            gotsync=preambledetector.Update(bit);
          }

        if(cntr<1000000000)cntr++;
        if(cntr<16)
        {
            if(cntr==0)
            {
                frameinfo=bit;
                infofield.clear();
                if(burstmode)//for R and T channels there are no headers so just fill in a dummy one
                {
                    formatid=1;
                    supfrmaker=0;
                    framecounter1=0;
                    framecounter2=0;
                    cntr=16;

                    if(rtchanneldeleavefecscram.resetblockptr()==RTChannelDeleaveFECScram::Bad_Packet)
                    {
                        decodedbytes+=" Bad R/T Packet\n";

                    }
                }
            }
            else
            {
                frameinfo<<=1;
                frameinfo|=bit;
            }
        }
        if(cntr==15)
        {
            //decodedbytes.push_back('\n');

            //decodedbytes.append(((QString)"%1\n").arg(frameinfo));

            //delay frame info by one frame needed as viterbi decoder and delayline make 1 frame delay
            quint16 tval=frameinfo;
            frameinfo=lastframeinfo;
            lastframeinfo=tval;

            //if(framecounter1!=framecounter2)decodedbytes.push_back("Error: Frame Counter mismatch\n");
            // else decodedbytes.push_back((((QString)"Format ID = %1\nSuper Frame Marker = %2\nFrame Counter = %3\n").arg(formatid).arg(supfrmaker).arg(framecounter1)).toLatin1());

            formatid=(frameinfo>>12)&0x000F;
            supfrmaker=(frameinfo>>8)&0x000F;
            framecounter1=(frameinfo>>4)&0x000F;
            framecounter2=(frameinfo>>0)&0x000F;

            //decodedbytes.push_back((((QString)"Last info is Format ID = %1\nSuper Frame Marker = %2\nFrame Counter = %3\n").arg(formatid).arg(supfrmaker).arg(framecounter1)).toLatin1());
        }
        if(cntr>=16)
        {

            //do rt decoding
            if(burstmode)
            {

                RTChannelDeleaveFECScram::ReturnResult result;

                if(useingOQPSK)
                {
                    if(soft)result = rtchanneldeleavefecscram.update(soft_bit);
                     else result = rtchanneldeleavefecscram.update(bit);
                }
                 else
                 {
                    if(soft)result = rtchanneldeleavefecscram.updateMSK(soft_bit);
                     else result = rtchanneldeleavefecscram.updateMSK(bit);
                 }

                switch(result)
                {
                case RTChannelDeleaveFECScram::OK_R_Packet:
                {

                    //processing R channel packets
                    QString decline;
                    for(int j=0;j<19-2;j++)
                    {
                        decline+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)rtchanneldeleavefecscram.infofield[j]));
                    }
                    decline+=" ";

                    using namespace AEROTypeR;
                    MessageType message=(MessageType)((uchar)rtchanneldeleavefecscram.infofield[2]);
                    if((((uchar)rtchanneldeleavefecscram.infofield[1])&0x08)==0x08)message=User_data_ISU_SSU_R_channel;

                    switch(message)
                    {
                    case General_access_request_telephone:
                        decline+="General_access_request_telephone";
                        break;
                    case Abbreviated_access_request_telephone:
                        decline+="Abbreviated_access_request_telephone";
                        break;
                    case Access_request_data_R_T_channel:
                        decline+="Access_request_data_R_T_channel";
                        break;
                    case Request_for_acknowledgement_R_channel:
                        decline+="Request_for_acknowledgement_R_channel";
                        break;
                    case Acknowledgement_R_channel:
                        decline+="Acknowledgement_R_channel";
                        break;
                    case Log_On_Off_control_R_channel:
                        decline+="Log_On_Off_control_R_channel";
                        break;
                    case Call_progress_R_channel:
                        decline+="Call_progress_R_channel";
                        break;
                    case Log_On_Off_acknowledgement:
                        decline+="Log_On_Off_acknowledgement";
                        break;
                    case Log_control_R_channel_ready_for_reassignment:
                        decline+="Log_control_R_channel_ready_for_reassignment";
                        break;
                    case Telephony_acknowledge_R_channel:
                        decline+="Telephony_acknowledge_R_channel";
                        break;
                    case User_data_ISU_SSU_R_channel:
                        decline+="User_data_ISU_SSU_R_channel";

                        if(risudata.update(rtchanneldeleavefecscram.infofield.left(17)))
                        {
                            parserisu->downlink=burstmode;
                            parserisu->parse(risudata.lastvalidisuitem);//emits signals if it find valid acars data or errors
                        }

                        int byte1=((uchar)rtchanneldeleavefecscram.infofield[-1+1]);
                        int byte2=((uchar)rtchanneldeleavefecscram.infofield[-1+2]);
                        int byte3=((uchar)rtchanneldeleavefecscram.infofield[-1+3]);
                        int byte4=((uchar)rtchanneldeleavefecscram.infofield[-1+4]);
                        int byte5=((uchar)rtchanneldeleavefecscram.infofield[-1+5]);
                        int byte6=((uchar)rtchanneldeleavefecscram.infofield[-1+6]);

                        uchar SEQINDICATOR=((byte1&0xF0)>>4);
                        uchar SUTYPE=byte1&0x0F;
                        quint32 AESID=byte3<<8*2|byte4<<8*1|byte5<<8*0;
                        int GES=byte6;

                        int SUTotal=0;
                        int SUindex=0;
                        switch(SEQINDICATOR)
                        {
                        case 1:
                            SUTotal=1;
                            SUindex=0;
                            break;
                        case 2:
                            SUTotal=2;
                            SUindex=0;
                            break;
                        case 3:
                            SUTotal=2;
                            SUindex=1;
                            break;
                        case 4:
                            SUTotal=3;
                            SUindex=0;
                            break;
                        case 5:
                            SUTotal=3;
                            SUindex=1;
                            break;
                        case 6:
                            SUTotal=3;
                            SUindex=2;
                            break;
                        default:
                            break;
                        }

                        int BytesInSU=0;
                        if((SUTYPE>=1)&&(SUTYPE<=11))BytesInSU=SUTYPE;

                        decline+=((QString)" SU %1 of %2. AES: %3 GES: %4").arg(SUindex+1).arg(SUTotal).arg((((QString)"%1").arg(AESID,6, 16, QChar('0'))).toUpper()).arg((((QString)"%1").arg(GES,2, 16, QChar('0'))).toUpper())   ;
                        /*decline+=" \"";
                            for(int m=6;m<17;m++)
                            {
                                int byte=((uchar)infofield[k*bytesinsu+m]);
                                byte&=0x7F;//8th bit is parity check?
                                if((byte>=0x20)&&(byte<=0x7E))
                                {
                                    decline+=(char)byte;
                                }
                                 else decline+=" ";
                            }
                            decline+="\"";*/

                           break;
                    }

                    decline+='\n';
                    if((message==User_data_ISU_SSU_R_channel)&&(donotdisplaysus.contains(0xC0)))decline.clear();
                    else if(donotdisplaysus.contains(message))decline.clear();//do not display these SUs as given to us by user

                    decodedbytes+=decline;

                   }
                    break;
                case RTChannelDeleaveFECScram::OK_T_Packet:
                {

                    using namespace AEROTypeT;
                    int byte1=((uchar)rtchanneldeleavefecscram.infofield[-1+1]);
                    int byte2=((uchar)rtchanneldeleavefecscram.infofield[-1+2]);
                    int byte3=((uchar)rtchanneldeleavefecscram.infofield[-1+3]);
                    int byte4=((uchar)rtchanneldeleavefecscram.infofield[-1+4]);

                    QString decline;
                    quint32 AESID=byte1<<8*2|byte2<<8*1|byte3<<8*0;
                    int GES=byte4;
                    decline+=((QString)" T Packet from AES: %3 to GES: %4 with %5 SUs\n").arg((((QString)"%1").arg(AESID,6, 16, QChar('0'))).toUpper()).arg((((QString)"%1").arg(GES,2, 16, QChar('0'))).toUpper()).arg(rtchanneldeleavefecscram.numberofsus);
                    decodedbytes+=decline;decline.clear();

                    for(int k=0;k<rtchanneldeleavefecscram.numberofsus;k++)
                    {
                        for(int j=0;j<12-2;j++)
                        {
                            decline+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)rtchanneldeleavefecscram.infofield[6+k*12+j]));
                        }

                        MessageType message=(MessageType)((uchar)rtchanneldeleavefecscram.infofield[6+k*12]);
                        if((message&0xC0)==0xC0)message=User_data_ISU_SSU_T_channel;
                        switch(message)
                        {
                        case Fill_in_signal_unit:
                            decline+=" Fill_in_signal_unit";
                            break;
                        case User_data_ISU_RLS_T_channel:
                            decline+=" User_data_ISU_RLS_T_channel";
                            isudata.update(rtchanneldeleavefecscram.infofield.mid(6+k*12,10));
                            break;
                        case User_data_ISU_SSU_T_channel:
                            decline+=" User_data_ISU_SSU_T_channel";
                            if(isudata.update(rtchanneldeleavefecscram.infofield.mid(6+k*12,10)))
                            {
                                parserisu->downlink=burstmode;
                                parserisu->parse(isudata.lastvalidisuitem);//emits signals if it find valid acars data or errors
                            }
                            else if(isudata.missingssu)decline+=" missing (or if unimplimented then TODO in C++)";
                            break;

                        }

                        decline+="\n";

                        if((message==User_data_ISU_SSU_T_channel)&&(donotdisplaysus.contains(0xC0)))decline.clear();
                        else if(donotdisplaysus.contains(message))decline.clear();//do not display these SUs as given to us by user

                        decodedbytes+=decline;decline.clear();

                    }// done with this burst, reset data counters



                }
                    break;
                case RTChannelDeleaveFECScram::Bad_Packet:
                    decodedbytes+=" Bad R/T Packet\n";
                    break;
                default:
                    break;
                }

            }

             else
             {

                // its a p channel

                //fill block
                if(cntr==16)blockcnt=-1;
                int idx=(cntr-AERO_SPEC_BitsInHeader)%block.size();
                if(idx<0)idx=0;//for dummy bits drop
                block[idx]=soft_bit;

                if(idx==(block.size()-1))//block is now filled
                {

                    blockcnt++;

                    //deinterleaver
                    QByteArray deleaveredblockBA=leaver.deinterleave_ba(block, 0);

                    QVector<int> deconvol=jconvolcodec->Decode_Continuous(deleaveredblockBA);

                   //delay line for frame alignment for non burst modes. This is needed for the scrambler
                    dl2.update(deconvol);

                    //scrambler
                    scrambler.update(deconvol);

                    //pack the bits into bytes
                    int charptr=0;
                    uchar ch=0;
                    for(int h=0;h<deconvol.size();h++)
                    {
                        ch|=deconvol[h]*128;
                        charptr++;charptr%=8;
                        if(charptr==0)
                        {
                            infofield+=ch;//actual data of information field in bytearray
                            ch=0;
                        }
                        else ch>>=1;
                    }

                    if((cntr-AERO_SPEC_BitsInHeader)==(AERO_SPEC_NumberOfBits-1))//frame is done when this is true
                    {

                        //run through all SUs and check CRCs
                        char *infofieldptr=infofield.data();
                        QString decline;
                        if(formatid!=1)decline+="format ID error\n";
                        for(int k=0;k<infofield.size()/12;k++)
                        {
                            quint16 crc_calc=crc16.calcusingbytes(&infofieldptr[k*12],12-2);
                            quint16 crc_rec=(((uchar)infofield[k*12+12-1])<<8)|((uchar)infofield[k*12+12-2]);

                            if((!crc_rec)&&(crc_calc!=crc_rec))
                            {
                                int tsum=0;
                                for(int ii=0; ii<(12-2); ii++)tsum+=(uchar)infofieldptr[k*12+ii];
                                if(tsum==0)crc_calc=0;//some sus are just zeros
                            }

                            //keep track of the DCD for non burst modes
                            if(crc_calc==crc_rec){if(datacdcountdown<12)datacdcountdown+=2;}
                            else {if(datacdcountdown>0)datacdcountdown-=3;}
                            if(!datacd&&datacdcountdown>2)
                            {
                                datacd=true;
                                emit DataCarrierDetect(datacd);
                            }

                            decline+=(k+'0');//SU number in frame
                            for(int j=0;j<12-2;j++)
                            {
                                decline+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)infofield[k*12+j]));
                            }
                            //                        decline+=((QString)"").sprintf(" rec = %04X calc = %04X", crc_rec,crc_calc);
                            if(crc_calc==crc_rec)
                            {

                                //                            decline+=" OK ";
                                decline+=" ";

                                //processing P channel packets
                                using namespace AEROTypeP;
                                MessageType message=(MessageType)((uchar)infofield[k*12]);
                                switch(message)
                                {
                                case Reserved_0:
                                    decline+="Reserved_0";
                                    break;
                                case Fill_in_signal_unit:
                                    decline+="Fill_in_signal_unit";
                                    break;
                                case AES_system_table_broadcast_GES_Psmc_and_Rsmc_channels_COMPLETE:
                                    decline+="AES_system_table_broadcast_GES_Psmc_and_Rsmc_channels_COMPLETE";
                                {
                                    int byte3=((uchar)infofield[k*12-1+3]);
                                    int ges=((uchar)infofield[k*12-1+4]);

                                    int byte5=((uchar)infofield[k*12-1+5]);
                                    int byte6=((uchar)infofield[k*12-1+6]);

                                    int byte7=((uchar)infofield[k*12-1+7]);
                                    int byte8=((uchar)infofield[k*12-1+8]);

                                    int byte9=((uchar)infofield[k*12-1+9]);
                                    int byte10=((uchar)infofield[k*12-1+10]);

                                    int channel1=(((byte5<<8)&0xFF00)|(byte6&0x00FF));
                                    int channel2=(((byte7<<8)&0xFF00)|(byte8&0x00FF));
                                    int channel3=(((byte9<<8)&0xFF00)|(byte10&0x00FF));
                                    double freq1=(((double)channel1)*0.0025)+1510.0;
                                    double freq2=(((double)channel2)*0.0025)+1510.0;
                                    double freq3=(((double)channel3)*0.0025)+1510.0;

                                    int seqno=(byte3>>2)&0x3F;
                                    int lsu=byte3&0x03;

                                    if(lsu<=1)
                                    {
                                        freq2+=101.5;
                                        freq3+=101.5;
                                        decline+=((QString)" Seq = %1 ").arg(seqno)+(("GES = "+(((QString)"%1").arg(ges, 2, 16, QChar('0')).toUpper()))+((QString)" --> Psmc  = %1MHz (RX), Rsmc0 = %2MHz (TX), Rsmc1 = %3MHz (TX)").arg(freq1,0, 'f', 4).arg(freq2,0, 'f', 4).arg(freq3,0, 'f', 4));
                                    }
                                    else if(lsu==2)
                                    {
                                        freq1+=101.5;
                                        freq2+=101.5;
                                        freq3+=101.5;
                                        decline+=((QString)" Seq = %1 ").arg(seqno)+(("GES = "+(((QString)"%1").arg(ges, 2, 16, QChar('0')).toUpper()))+((QString)" --> Rsmc2 = %1MHz (TX), Rsmc3 = %2MHz (TX), Rsmc4 = %3MHz (TX)").arg(freq1,0, 'f', 4).arg(freq2,0, 'f', 4).arg(freq3,0, 'f', 4));
                                    }
                                    else if(lsu==3)
                                    {
                                        freq1+=101.5;
                                        freq2+=101.5;
                                        freq3+=101.5;
                                        decline+=((QString)" Seq = %1 ").arg(seqno)+(("GES = "+(((QString)"%1").arg(ges, 2, 16, QChar('0')).toUpper()))+((QString)" --> Rsmc5 = %1MHz (TX), Rsmc6 = %2MHz (TX), Rsmc7 = %3MHz (TX)").arg(freq1,0, 'f', 4).arg(freq2,0, 'f', 4).arg(freq3,0, 'f', 4));
                                    }

                                }
                                    break;
                                case AES_system_table_broadcast_GES_beam_support_COMPLETE:
                                    decline+="AES_system_table_broadcast_GES_beam_support_COMPLETE";
                                    break;
                                case AES_system_table_broadcast_index:
                                    decline+="AES_system_table_broadcast_index";
                                    break;
                                case AES_system_table_broadcast_satellite_identification_COMPLETE:
                                    decline+="AES_system_table_broadcast_satellite_id_COMPLETE";
                                {
                                    int byte3=((uchar)infofield[k*12-1+3]);
                                    int byte4=((uchar)infofield[k*12-1+4]);

                                    //int ra=((uchar)infofield[k*12-1+5]);//ra
                                    int byte6=((uchar)infofield[k*12-1+6]);//long
                                    double longitude=((double)byte6)*1.5;

                                    int byte7=((uchar)infofield[k*12-1+7]);
                                    int byte8=((uchar)infofield[k*12-1+8]);

                                    int byte9=((uchar)infofield[k*12-1+9]);
                                    int byte10=((uchar)infofield[k*12-1+10]);

                                    int channel1=((((byte7&0x7F)<<8)&0xFF00)|(byte8&0x00FF));
                                    int channel2=((((byte9&0x7F)<<8)&0xFF00)|(byte10&0x00FF));
                                    double cac_freq1=(((double)channel1)*0.0025)+1510.0;
                                    double cac_freq2=(((double)channel2)*0.0025)+1510.0;

                                    QString spotbeam1qstr;
                                    QString spotbeam2qstr;
                                    if(byte7&0x80)spotbeam1qstr=" (Spot beam)";
                                    if(byte9&0x80)spotbeam2qstr=" (Spot beam)";

                                    //int inc=(byte4>>1)&0x07;


                                    int seqno=(byte3>>2)&0x3F;
                                    int satid=(((byte3<<4)&0x30)|((byte4>>4)&0x0F));
                                    QString longitude_qstr;
                                    if(longitude>180.0)longitude_qstr=(QString("%1W")).arg(360.0-longitude);
                                    else longitude_qstr=(QString("%1E")).arg(longitude);
                                    if(channel2!=0)decline+=((QString)" SATELLITE ID = %1 (Long %3) Seq = %2 Psmc1 = %4MHz%6 Psmc2 = %5MHz%7").arg(satid).arg(seqno).arg(longitude_qstr).arg(cac_freq1,0, 'f', 4).arg(cac_freq2,0, 'f', 4).arg(spotbeam1qstr).arg(spotbeam2qstr);
                                    else decline+=((QString)" SATELLITE ID = %1 (Long %3) Seq = %2  Psmc1 = %4MHz%5").arg(satid).arg(seqno).arg(longitude_qstr).arg(cac_freq1,0, 'f', 4).arg(spotbeam1qstr);
                                    //if(channel2!=0)decline+=((QString)" SATELLITE ID = %1 (Long %3 RA %6 INC %7) Seq = %2 Psmc1 = %4MHz Psmc2 = %5MHz").arg(satid).arg(seqno).arg(longitude_qstr).arg(cac_freq1,0, 'f', 3).arg(cac_freq2,0, 'f', 3).arg(ra).arg(inc);
                                    // else decline+=((QString)" SATELLITE ID = %1 (Long %3 RA %5 INC %6) Seq = %2  Psmc1 = %4MHz").arg(satid).arg(seqno).arg(longitude_qstr).arg(cac_freq1,0, 'f', 3).arg(ra).arg(inc);
                                }
                                    break;

                                    //SYSTEM LOG-ON/LOG-OFF
                                case Log_on_request:
                                    decline+="Log_on_request";
                                    break;
                                case Log_on_confirm:
                                    decline+="Log_on_confirm";

                                {
                                    SendLogOnOff(k, "Log on confirm");
                                }
                                    break;
                                case Log_control_P_channel_log_off_request:
                                    decline+="Log_control_P_channel_log_off_request";
                                    break;
                                case Log_control_P_channel_log_on_reject:
                                    decline+="Log_control_P_channel_log_on_reject";
                                    break;
                                case Log_control_P_channel_log_on_interrogation:
                                    decline+="Log_control_P_channel_log_on_interrogation";

                                    break;
                                case Log_on_log_off_acknowledge_P_channel:
                                    decline+="Log_on_log_off_acknowledge_P_channel";
                                    break;
                                case Log_control_P_channel_log_on_prompt:
                                    decline+="Log_control_P_channel_log_on_prompt";
                                    break;
                                case Log_control_P_channel_data_channel_reassignment:
                                    decline+="Log_control_P_channel_data_channel_reassignment";
                                    break;

                                case Reserved_18:
                                    decline+="Reserved_18";
                                    break;
                                case Reserved_19:
                                    decline+="Reserved_19";
                                    break;
                                case Reserved_26:
                                    decline+="Reserved_26";
                                    break;

                                    //CALL INITIATION
                                case Data_EIRP_table_broadcast_complete_sequence:
                                    decline+="Data_EIRP_table_broadcast_complete_sequence";
                                    break;

                                case T_channel_assignment:
                                    decline+="T_channel_assignment";
                                    break;

                                case C_channel_assignment_distress:
                                    decline+="C_channel_assignment_distress";
                                {
                                    CChannelAssignmentItem item=CreateCAssignmentItem(infofield.mid(k*12,12));
                                    emit CChannelAssignmentSignal(item);
                                    SendCAssignment(k, decline);
                                }
                                break;

                                case C_channel_assignment_flight_safety:
                                    decline+="C_channel_assignment_flight_safety";
                                {
                                    CChannelAssignmentItem item=CreateCAssignmentItem(infofield.mid(k*12,12));
                                    emit CChannelAssignmentSignal(item);
                                    SendCAssignment(k, decline);
                                }
                                break;

                                case C_channel_assignment_other_safety:
                                    decline+="C_channel_assignment_other_safety";
                                {
                                    CChannelAssignmentItem item=CreateCAssignmentItem(infofield.mid(k*12,12));
                                    emit CChannelAssignmentSignal(item);
                                    SendCAssignment(k, decline);
                                }
                                break;

                                case C_channel_assignment_non_safety:
                                    decline+="C_channel_assignment_non_safety";
                                {
                                    CChannelAssignmentItem item=CreateCAssignmentItem(infofield.mid(k*12,12));
                                    emit CChannelAssignmentSignal(item);
                                    SendCAssignment(k, decline);
                                }
                                break;


                                case Call_announcement:
                                    decline+="Call_announcement";
                                {

                                    SendCAssignment(k, decline);
                                }
                                break;

                               //CHANNEL INFORMATION
                                case P_R_channel_control_ISU:
                                    decline+="P_R_channel_control_ISU";
                                {
                                    //int byte3=((uchar)infofield[k*12-1+3]);
                                    //int byte4=((uchar)infofield[k*12-1+4]);

                                    int ges=((uchar)infofield[k*12-1+5]);
                                    //int byte6=((uchar)infofield[k*12-1+6]);

                                    //int byte7=((uchar)infofield[k*12-1+7]);
                                    int byte8=((uchar)infofield[k*12-1+8]);

                                    int byte9=((uchar)infofield[k*12-1+9]);
                                    int byte10=((uchar)infofield[k*12-1+10]);


                                    int channel=((((byte9&0x7F)<<8)&0xFF00)|(byte10&0x00FF));
                                    double freq=(((double)channel)*0.0025)+1510.0;
                                    bool spotbeam=false;
                                    if(byte9&0x80)spotbeam=true;

                                    int bitrate=(byte8>>4)&0x0F;
                                    //int nooffreqs=(byte7>>4)&0x0F;

                                    switch(bitrate)
                                    {
                                    case 0:
                                        bitrate=600;
                                        break;
                                    case 1:
                                        bitrate=1200;
                                        break;
                                    case 2:
                                        bitrate=2400;
                                        break;
                                    case 3:
                                        bitrate=4800;
                                        break;
                                    case 4:
                                        bitrate=6000;
                                        break;
                                    case 5:
                                        bitrate=5250;
                                        break;
                                    case 6:
                                        bitrate=10500;
                                        break;
                                    case 7:
                                        bitrate=8400;
                                        break;
                                    case 9:
                                        bitrate=21000;
                                        break;
                                    default:
                                        bitrate=-1;
                                        break;
                                    }

                                    //decline+=((" GES = "+(((QString)"%1").arg(ges, 2, 16, QChar('0')).toUpper()))+((QString)" Pd = %1MHz at %2bps with %3 frequencies assigned").arg(freq,0, 'f', 3).arg(bitrate).arg(nooffreqs));
                                    if(spotbeam)decline+=((" GES = "+(((QString)"%1").arg(ges, 2, 16, QChar('0')).toUpper()))+((QString)" Pd = %1MHz at %2bps (Spot beam)").arg(freq,0, 'f', 3).arg(bitrate));
                                    else decline+=((" GES = "+(((QString)"%1").arg(ges, 2, 16, QChar('0')).toUpper()))+((QString)" Pd = %1MHz at %2bps").arg(freq,0, 'f', 3).arg(bitrate));

                                }
                                    break;
                                case T_channel_control_ISU:
                                    decline+="T_channel_control_ISU";
                                    break;

                                    //ACKNOWLEDGEMENT
                                case Request_for_acknowledgement_RQA_P_channel:
                                    decline+="Request_for_acknowledgement_RQA_P_channel";
                                    break;
                                case Acknowledge_RACK_TACK_P_channel:
                                    decline+="Acknowledge_RACK_TACK_P_channel";
                                    break;

                                case User_data_ISU_RLS_P_T_channel:
                                    decline+="User_data_ISU_RLS_P_T_channel";
                                {
                                    /*quint32 AESID=((uchar)infofield[k*12+1])<<8*2|((uchar)infofield[k*12+2])<<8*1|((uchar)infofield[k*12+3])<<8*0;
                                    uchar GESID=(uchar)infofield[k*12+4];
                                    decodedbytes+=((QString)" AESID = %1 GESID = %2").arg(AESID).arg(GESID);*/
                                    isudata.update(infofield.mid(k*12,10));
                                }
                                    break;
                                case User_data_3_octet_LSDU_RLS_P_channel:
                                    decline+="User_data_3_octet_LSDU_RLS_P_channel";
                                    break;
                                case User_data_4_octet_LSDU_RLS_P_channel:
                                    decline+="User_data_4_octet_LSDU_RLS_P_channel";
                                    break;
                                default:
                                    if((message&0xC0)==0xC0)
                                    {
                                        decline+="SSU";

                                        if(isudata.update(infofield.mid(k*12,10)))
                                        {
                                            parserisu->downlink=burstmode;
                                            parserisu->parse(isudata.lastvalidisuitem);//emits signals if it find valid acars data or errors
                                        }
                                        else if(isudata.missingssu)decline+=" missing (or if unimplimented then TODO in C++)";

                                        /*                                    decodedbytes+="SUBSEQUENT SIGNAL UNITS";
                                    decodedbytes+=" \"";
                                    for(int m=2;m<10;m++)
                                    {
                                        int byte=((uchar)infofield[k*12+m]);
                                        byte&=0x7F;//8th bit is parity check?
                                        if((byte>=0x20)&&(byte<=0x7E))
                                        {
                                            decodedbytes+=(char)byte;
                                        }
                                         else decodedbytes+=" ";
                                    }
                                    decodedbytes+="\"";
*/
                                    }
                                    break;
                                }


                                decline+='\n';
                                if(((message&0xC0)==0xC0)&&(donotdisplaysus.contains(0xC0)))decline.clear();
                                else if(donotdisplaysus.contains(message))decline.clear();//do not display these SUs as given to us by user




                            }
                             else
                             {
                                decline+=" Bad CRC\n";
                                //allgood=false;
                             }

                            /*if(crc_calc==crc_rec)qDebug()<<k<<((QString)"").sprintf("rec = %02X", crc_rec)<<((QString)"").sprintf("calc = %02X", crc_calc)<<"OK"<<unencoded_BER_estimate*100.0;
                         else
                         {
                            allgood=false;
                            qDebug()<<k<<((QString)"").sprintf("rec = %02X", crc_rec)<<((QString)"").sprintf("calc = %02X", crc_calc)<<"Bad CRC"<<unencoded_BER_estimate*100.0;
                         }
                         */

                            //if(!decline.isEmpty())qDebug()<<decline;

                            decodedbytes+=decline;
                            decline.clear();

                        }

                        //if(!allgood)decodedbytes+="Got at least one bad SU (a SU failed CRC check)\n";//tell the console



                    }

                }


             }


        }


        if(gotsync)
        {

            if(!burstmode)
            {
                if(cntr+1!=AERO_SPEC_TotalNumberOfBits)
                {
                    isudata.reset();
                    decodedbytes+="Error short frame!!! maybe the soundcard dropped some sound card buffers\n";
                }
            }

            //            decodedbytes+=((QString)"Bits for frame = %1\n").arg(cntr+1);
            cntr=-1;

            //got a signal
            datacd=true;
            datacdcountdown=12;
            emit DataCarrierDetect(datacd);

            scrambler.reset();
        }

        if(cntr+1==AERO_SPEC_TotalNumberOfBits)
        {
            scrambler.reset();
            cntr=-1;

            if(burstmode)
            {

               //end of signal. stop processing and send dcd low
                cntr=1000000000;//stop processing
                datacd=false;
                datacdcountdown=0;
                emit DataCarrierDetect(datacd);
                return decodedbytes;
            }
        }


    }

    if((!datacd)&&(!burstmode))
    {
        decodedbytes.clear();
    }

    return decodedbytes;
}

qint64 AeroL::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}

qint64 AeroL::writeData(const char *data, qint64 len)
{
    sbits.clear();
    uchar *udata=(uchar*)data;
    uchar auchar;
    for(int i=0;i<len;i++)
    {
        auchar=udata[i];
        if(preambledetectorburst.Update(auchar))
        {
            sbits.push_back(-1);
        }
        else for(int j=0;j<8;j++)
        {
            sbits.push_back(auchar&1);
            auchar=auchar>>1;
        }
    }
    Decode(sbits);

    if(!psinkdevice.isNull())
    {
        QIODevice *out=psinkdevice.data();
        if(out->isOpen())out->write(decodedbytes);
    }

    return len;
}

void AeroL::processDemodulatedSoftBits(const QVector<short> &soft_bits)
{

    sbits.clear();

    sbits.append(soft_bits);

    if(this->ifb == 8400)
    {
       DecodeC(sbits);
    }else
    {
       Decode(sbits, true);
    }

    if(!psinkdevice.isNull())
    {
        QIODevice *out=psinkdevice.data();
        if(out->isOpen())out->write(decodedbytes);
    }


}

CChannelAssignmentItem AeroL::CreateCAssignmentItem(QByteArray su)
{
    CChannelAssignmentItem item;

    using namespace AEROTypeP;
    switch((uchar)su[0])
    {
    case C_channel_assignment_distress:
        break;
    case C_channel_assignment_flight_safety:
        break;
    case C_channel_assignment_other_safety:
        break;
    case C_channel_assignment_non_safety:
        break;
    default:
        qDebug()<<"unknown CChannelAssignment type"<<(uchar)su[0];
        //return item; // dont return in case this is a reserved type. however without this return I wouldn't have figured out that there was a bug calling this function.
    }

    item.type=(uchar)su[0];

    item.AESID=((uchar)su[-1+2])<<8*2|((uchar)su[-1+3])<<8*1|((uchar)su[-1+4])<<8*0;
    item.GESID=su[-1+5];

    int byte7=((uchar)su[-1+7]);
    int byte8=((uchar)su[-1+8]);
    int byte9=((uchar)su[-1+9]);
    int byte10=((uchar)su[-1+10]);

    int channel_rx=((((byte7&0x7F)<<8)&0xFF00)|(byte8&0x00FF));
    int channel_tx=((((byte9&0x7F)<<8)&0xFF00)|(byte10&0x00FF));
    item.receive_freq=(((double)channel_rx)*0.0025)+1510.0;
    item.transmit_freq=(((double)channel_tx)*0.0025)+1611.5;
    if(byte7&0x80)item.receive_spotbeam=true;
    if(byte9&0x80)item.transmit_spotbeam=true;


    //looking up the db for plane info is a hassle so i'm not doing it at the moment.
    //probably something to look at later

    return item;
}

void AeroL::SendCAssignment(int k, QString decline)
{
    ACARSItem item;
    item.isuitem.AESID=((uchar)infofield[k*12-1+2])<<8*2|((uchar)infofield[k*12-1+3])<<8*1|((uchar)infofield[k*12-1+4])<<8*0;
    item.isuitem.GESID=infofield[k*12-1+5];

    item.hastext = true;
    item.downlink = true;
    item.nonacars = true;
    item.valid = true;
    int byte7=((uchar)infofield[k*12-1+7]);
    int byte8=((uchar)infofield[k*12-1+8]);
    int byte9=((uchar)infofield[k*12-1+9]);
    int byte10=((uchar)infofield[k*12-1+10]);

    int channel1=((((byte7&0x7F)<<8)&0xFF00)|(byte8&0x00FF));
    int channel2=((((byte9&0x7F)<<8)&0xFF00)|(byte10&0x00FF));
    double rx=(((double)channel1)*0.0025)+1510.0;
    double tx=(((double)channel2)*0.0025)+1611.5;
    QString receive = QString::number(rx, 'f', 4);
    QString transmit = QString::number(tx, 'f', 4);
    QString beam = " Global Beam ";
    if(byte7&0x80)beam=" Spot Beam ";

    item.message = "Receive Freq: " + receive + beam + "Transmit " + transmit + "\r\n" + decline;
    emit ACARSsignal(item);
}
void AeroL::SendLogOnOff(int k, QString text)
{
    ACARSItem item;
    item.isuitem.AESID=((uchar)infofield[k*12-1+2])<<8*2|((uchar)infofield[k*12-1+3])<<8*1|((uchar)infofield[k*12-1+4])<<8*0;
    item.isuitem.GESID=infofield[k*12-1+5];

    item.hastext = true;
    item.downlink = true;
    item.nonacars = true;
    item.valid = true;

    item.message = text;
    emit ACARSsignal(item);
}

QByteArray &AeroL::DecodeC(QVector<short> &bits)
{

    decodedbytes.clear();



    quint16 bit=0;
    quint16 soft_bit=0;

    QString hex = "000000";

    for(int i=0;i<bits.size();i++)
    {

        // hard bits for preamble
        if(((uchar)bits[i])>=128)bit=1;
        else bit=0;
        soft_bit=bits[i];
        int gotsync = 0;

        realimag++;realimag%=2;
        if(realimag)
        {

            if(cntr > AERO_SPEC_NumberOfBits-112 || cntr <= 0)
            {


            gotsync=preambledetectorreal.Update(bit);
            if(!gotsync_last)
            {
                gotsync_last=gotsync;
                gotsync=0;
            } else gotsync_last=0;

            } else {
                gotsync = 0;
                gotsync_last = 0;

            }

        }
         else
         {

            if(cntr > AERO_SPEC_NumberOfBits-112 || cntr <= 0)
            {

            gotsync=preambledetectorimag.Update(bit);
            if(!gotsync_last)
            {
                gotsync_last=gotsync;
                gotsync=0;
            } else gotsync_last=0;
            } else {
                gotsync = 0;
                gotsync_last = 0;

            }



         }

        if(realimag)
        {
            if(preambledetectorreal.inverted)
            {
                bit=1-bit;

                if(soft_bit > 128)
                {
                     soft_bit = 255-soft_bit;
                } else if (soft_bit < 128)
                {
                    soft_bit = 255-soft_bit;
                }
            }

        }
        else
        {
            if(preambledetectorimag.inverted)
            {
                bit=1-bit;

                if(soft_bit > 128)
                {
                     soft_bit = 255-soft_bit;
                }
                else if (soft_bit < 128)
                {
                     soft_bit = 255-soft_bit;
                }
            }
        }
        if(gotsync)
        {

            // reset the counter, add first bit next time around
            cntr=-1;
            index=-1;

            deleaveredBlock.resize(0);
            depuncturedBlock.clear();
            scrambler.reset();
        }
        else
        {
           // start adding bits to the buffer
           // frame should be complete

            if(cntr<1000000000)cntr++;
            if(cntr<=AERO_SPEC_NumberOfBits-1)
            {
                index++;
                block[index]=soft_bit;
            }
            if(index == (255))
            {
                //deinterleave block
                QByteArray deleaveredblockBA=leaver.deinterleave_ba(block, 4);

                //apend deinterleaved block to total frame
                deleaveredBlock.append(deleaveredblockBA);

                // reset for next deleaver block
                index = -1;

            }
            if(cntr==AERO_SPEC_NumberOfBits-1)
            {

                // depuncture the full block
                puncturedCode.depunture_soft_block(deleaveredBlock, depuncturedBlock, 4, true);

                QVector<int> deconvol=jconvolcodec->Decode_Continuous(depuncturedBlock);

                //resize to drop trailing dummy bits
                deconvol.resize(2714);

                //delay line for frame alignment for non burst modes. This is needed for the scrambler
                dl2.update(deconvol);

                //scrambler
                scrambler.update(deconvol);

                //pack the bits into bytes
                infofield.clear();
                int charptr=0;
                uchar ch=0;

                // extract the 24 sub data 12 bit blocks

                for(int y=0; y<24;y++)
                {
                    // offset from start of frame
                    int offset = y * (1+96+12);

                    for(int h=offset+97;h<offset+109;h++)
                    {
                        ch|=deconvol[h]*128;
                        charptr++;charptr%=8;
                        if(charptr==0)
                        {
                            infofield+=ch;//actual data of information field in bytearray
                            ch=0;
                        }
                        else ch>>=1;
                    }

                    if(infofield.size()==12)
                    {

                        QString decline;

                        bool crcok = false;

                        char *infofieldptr=infofield.data();
                        quint16 crc_calc=crc16.calcusingbytes(&infofieldptr[0],12-2);
                        quint16 crc_rec=(((uchar)infofield[12-1])<<8)|((uchar)infofield[12-2]);

                        //keep track of the DCD for non burst modes
                        if(crc_calc==crc_rec)
                        {
                            crcok = true;
                            if(datacdcountdown<12)datacdcountdown+=2;
                        }
                        else
                        {
                            if(datacdcountdown>0)datacdcountdown-=5;//3;//C channel packets come slower so make timeout faster to compensate
                        }
                        if(!datacd&&datacdcountdown>2)
                        {
                            datacd=true;
                            emit DataCarrierDetect(datacd);

                            decline += " CRC Failed for C Channel Sub-band signal unit \r\n";
                        }

                        // check for signal unit
                        if(crcok)
                        {

                            using namespace AEROTypeC;

                            MessageType message=(MessageType)((uchar)infofield.at(0));
                            switch(message)
                            {


                            case Fill_in_signal_unit:
                                //decline+="Fill_in_signal_unit \r\n";
                                break;

                            case Call_progress:
                            {

                                for(int k=0;k<infofield.size()-2;k++)decline+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)infofield[k]));
                                decline+=" AES = "+infofield.mid(1,3).toHex().toUpper();
                                decline+=" GES = "+infofield.mid(4,1).toHex().toUpper();
                                decline+=" Call_progress \r\n";
                                emit Call_progress_Signal(infofield);

                                QString thex = infofield.mid(1,3).toHex().toUpper();

                                if(thex.length() == 6)
                                {
                                    hex = thex;
                                }

                            }
                            break;

                            case Telephony_acknowledge:
                            {

                                for(int k=0;k<infofield.size()-2;k++)decline+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)infofield[k]));
                                decline+=" AES = "+infofield.mid(1,3).toHex().toUpper();
                                decline+=" GES = "+infofield.mid(4,1).toHex().toUpper();
                                decline+=" Telephony_acknowledge \r\n";

                            }
                            break;

                            default:
                            {
                                for(int k=0;k<infofield.size()-2;k++)
                                {
                                    decline+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)infofield[k]));
                                }

                                decline+= " Other C Channel signal unit \r\n";
                            }
                            }
                        }

                        // set the response text for upper window

                        decodedbytes+=decline;
                        infofield.clear();
                    }


                }// end of sub-data loop

                // Lets get the voice data
                // extract the 24 sub data 12 bit blocks

                QByteArray data;
                data.reserve(300);

                int bitsin=0;
                for(int h=1; h<2714;h++)
                {

                        ch|=deconvol[h]*128;
                        charptr++;charptr%=8;
                        if(charptr==0)
                        {
                            data+=ch;//actual data of information field in bytearray
                            ch=0;
                        }
                        else ch>>=1;
                        bitsin++;
                        if(bitsin==96)
                        {

                            bitsin=0;
                            //move forward 13 bits;
                            h+=13;
                       }
                }

                // send for external decoding
                emit Voicesignal(data, hex);

                //25 primary fields. this is where the audio lives in a compressed format
                for(int i=0;i<25;i++)
                {
                     emit Voicesignal(data.mid(i*12,12));//send one frame at a time
                }

              // reset for next block
              index = -1;
            }// end of frame
        }
    }

    if(!datacd)
    {
        decodedbytes.clear();
    }

    return decodedbytes;
}

void PuncturedCode::depunture_soft_block(QByteArray &sourceblock,QByteArray &targetblock, int pattern,bool reset)
{
    assert(pattern>=2);

    if(reset)depunture_ptr=0;
    for(int i=0;i<sourceblock.size()-1;i++)
    {
        depunture_ptr++;
        targetblock.push_back(sourceblock.at(i));
        if(depunture_ptr>=pattern-1)targetblock.push_back(128);
        depunture_ptr%=(pattern-1);
    }

}

PuncturedCode::PuncturedCode()
{
    punture_ptr=0;
    depunture_ptr=0;
}


