#include "aerol.h"
#include <QtEndian>

using namespace AEROL;

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
        if(((anisuitem.SEQNO+1)==isuitems[i].SEQNO)&&(anisuitem.QNO==isuitems[i].QNO)&&(anisuitem.REFNO==isuitems[i].REFNO))return i;
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
    case User_data_ISU_RLS_P_T_channel:
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
        if(acarsitemexts[i].count>20)
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
//        qDebug()<<"pushed first acars fragment"<<anacarsitemext.anacarsitem.isuitem.AESID;
        return false;
    }

//    qDebug()<<"adding acars fragment"<<idx;

    //update frag
    ACARSItemext *polditem=&acarsitemexts[idx];
    polditem->count=0;
    polditem->anacarsitem.BI=acarsitem.BI;
    polditem->anacarsitem.message+=acarsitem.message;
    polditem->anacarsitem.moretocome=acarsitem.moretocome;

    //more to come then do nothing
    if(acarsitem.moretocome)return false;

//    qDebug()<<"last acars fragment"<<idx;

    //else return defragmented item with BI equal to last acars message
    acarsitem=polditem->anacarsitem;
    acarsitemexts.removeAt(idx);
    return true;


}

ParserISU::ParserISU(QObject *parent):
    QObject(parent)
{
    //
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
    //83 or 93 (ETX/ETB) * (ETB when text is over 220 chars)
    //93 AB (BSC no parity bits)
    //7F (DEL) *
    if((isuitem.userdata.size()>16)&&((uchar)isuitem.userdata[0])==0xFF&&((uchar)isuitem.userdata[1])==0xFF&&((uchar)isuitem.userdata[2])==0x01&&((((uchar)isuitem.userdata[isuitem.userdata.size()-1-3])==0x83)||(((uchar)isuitem.userdata[isuitem.userdata.size()-1-3])==0x97))&&((uchar)isuitem.userdata[isuitem.userdata.size()-1])==0x7F&&( ((uchar)isuitem.userdata[15])==0x83 || ((uchar)isuitem.userdata[15])==0x02 ))
    {

        //fill in header info
        anacarsitem.clear();
        anacarsitem.isuitem=isuitem;
        uchar byte=((uchar)isuitem.userdata[3]);
        anacarsitem.MODE=byte&0x7F;
        anacarsitem.TAK=((uchar)textish[11]);
        anacarsitem.LABEL+=((uchar)textish[12]);
        anacarsitem.LABEL+=((uchar)textish[13]);
        anacarsitem.BI=((uchar)textish[14]);
        if(((uchar)isuitem.userdata[15])==0x02)anacarsitem.hastext=true;
        if(((uchar)isuitem.userdata[isuitem.userdata.size()-1-3])==0x97)anacarsitem.moretocome=true;
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
                /*if(byte==10||byte==13)//replace CR and LF with a circle
                {
                    if(anacarsitem.message.right(1)=="●")continue;
                    anacarsitem.message+="●";
                }
                 else anacarsitem.message+=(char)byte;*/

                anacarsitem.message+=(char)byte;

            }
        }

        //mark as valid
        anacarsitem.valid=true;


//anacarsitem.MODE=4+'0';
//emit ACARSsignal(anacarsitem);
//anacarsitem.MODE=2+'0';


        //send acars message if fully defraged
        if(acarsdefragmenter.defragment(anacarsitem))emit ACARSsignal(anacarsitem);

        //send acars message
        //emit ACARSsignal(anacarsitem);

        return true;
    }

    return false;

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

AeroL::AeroL(QObject *parent) : QIODevice(parent)
{

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

    //ViterbiCodec is not Qt so needs deleting when finished
    std::vector<int> polynomials;
    polynomials.push_back(109);
    polynomials.push_back(79);
    convolcodec=new ViterbiCodec(7, polynomials);
    convolcodec->setPaddingLength(24);

    dl1.setLength(12);//delay for decode encode BER check
    dl2.setLength(576-6);//delay's data to next frame

    preambledetector.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011

    setBitRate(1200);

}

void AeroL::setBitRate(double fb)
{
    switch(qRound(fb))
    {
    case 600:
        leaver.setSize(6);//9 for 1200 bps, 6 for 600 bps
        block.resize(6*64);
        break;
    case 1200:
        leaver.setSize(9);//9 for 1200 bps, 6 for 600 bps
        block.resize(9*64);
        break;
    default:
        leaver.setSize(9);//9 for 1200 bps, 6 for 600 bps
        block.resize(9*64);
        break;
    }
}

AeroL::~AeroL()
{
    delete convolcodec;
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

QByteArray &AeroL::Decode(QVector<short> &bits)//0 bit --> oldest bit
{
    decodedbytes.clear();

    //static int cntr=1000000000;
    static int formatid=0;
    static int supfrmaker=0;
    static int framecounter1=0;
    static int framecounter2=0;

    for(int i=0;i<bits.size();i++)
    {



        if(cntr<1000000000)cntr++;
        if(cntr<16)
        {
            if(cntr==0)
            {
                frameinfo=bits[i];
                infofield.clear();
            }
             else
             {
                frameinfo<<=1;
                frameinfo|=bits[i];
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

            //fill block
            static int blockcnt=-1;
            if(cntr==16)blockcnt=-1;
            int idx=(cntr-16)%block.size();
            block[idx]=bits[i];
            if(idx==(block.size()-1))//block is now filled
            {

                blockcnt++;

                //deinterleaver
                QVector<int> deleaveredblock=leaver.deinterleave(block);

                //viterbi decoder
                QVector<int> deconvol=convolcodec->Decode_Continuous(deleaveredblock);

                //
                //Checking deinterleaving and viterbi decoder are getting valid data
                //
                //convol encode
                QVector<int> convol=convolcodec->Encode_Continuous(deconvol);
                //delay line for unencoded BER estimate
                dl1.update(deleaveredblock);
                //count differences
                assert(deleaveredblock.size()==convol.size());
                int diffsum=0;
                for(int k=0;k<deleaveredblock.size();k++)
                {
                    if(deleaveredblock[k]!=convol[k])diffsum++;
                }
                float unencoded_BER_estimate=((float)diffsum)/((float)deleaveredblock.size());
//                decodedbytes+=((QString)"unencoded BER estimate=%1%\n").arg(QString::number( 100.0*unencoded_BER_estimate, 'f', 1));

                //delay line for frame alignment
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
                        //decodedbytes+=((QString)"0x%1 ").arg(((QString)"").sprintf("%02X", ch)) ;//for console
                        ch=0;
                    }
                     else ch>>=1;
                }
                //decodedbytes+='\n';


                if((cntr-16)==(1152-1))//frame is done when this is true
                {

                    //run through all bytes in info field for console
                    //for(int k=0;k<infofield.size();k++)decodedbytes+=((QString)"0x%1 ").arg(((QString)"").sprintf("%02X", (uchar)infofield[k])) ;//for console
                    //decodedbytes+='\n';

                    //run through all SUs and check CRCs
                    //12 bytes for all SUs in P signal i think? or do extended SUs exist in the P signal????
                    char *infofieldptr=infofield.data();
                    //bool allgood=true;
//                    if(framecounter1==framecounter2)decodedbytes+=((QString)"").sprintf("Frame %d\n", framecounter1);
//                     else decodedbytes+="frame count error\n";
                    QString decline;
                    if(formatid!=1)decline+="format ID error\n";
                    for(int k=0;k<infofield.size()/12;k++)
                    {
                        quint16 crc_calc=crc16.calcusingbytes(&infofieldptr[k*12],10);
                        quint16 crc_rec=(((uchar)infofield[k*12+11])<<8)|((uchar)infofield[k*12+10]);

                        if((!crc_rec)&&(crc_calc!=crc_rec))
                        {
                            int tsum=0;
                            for(int ii=0; ii<10; ii++)tsum+=(uchar)infofieldptr[k*12+ii];
                            if(tsum==0)crc_calc=0;//some sus are just zeros
                        }

                        //keep track of the DCD
                        if(crc_calc==crc_rec){if(datacdcountdown<12)datacdcountdown+=2;}
                         else {if(datacdcountdown>0)datacdcountdown-=3;}
                        if(!datacd&&datacdcountdown>2)
                        {
                            datacd=true;
                            emit DataCarrierDetect(datacd);
                        }

                        MessageType message=(MessageType)((uchar)infofield[k*12]);



//if(message==0x26||message==0x0A)continue;//to skip the boring SUs

                        decline+=(k+'0');//SU number in frame
                        for(int j=0;j<10;j++)
                        {
                            decline+=((QString)" 0x%1").arg(((QString)"").sprintf("%02X", (uchar)infofield[k*12+j]));
                            //if(((uchar)infofield[j])>30&&((uchar)infofield[k*12+j])<127)decodedbytes+=(char)infofield[k*12+j];
                            // else decodedbytes+='.';
                        }
                        decline+=((QString)"").sprintf(" rec = %04X calc = %04X", crc_rec,crc_calc);
                        if(crc_calc==crc_rec)
                        {

                            decline+=" OK ";
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
                                break;
                            case AES_system_table_broadcast_GES_beam_support_COMPLETE:
                                decline+="AES_system_table_broadcast_GES_beam_support_COMPLETE";
                                break;
                            case AES_system_table_broadcast_index:
                                decline+="AES_system_table_broadcast_index";
                                break;
                            case AES_system_table_broadcast_satellite_identification_COMPLETE:
                                decline+="AES_system_table_broadcast_satellite_identification_COMPLETE";
                                {
                                    int byte3=((uchar)infofield[k*12-1+3]);
                                    int seqno=(byte3>>2)&0x3F;
                                    int satid=byte3&0x03;
                                    decline+=((QString)" SATELLITE ID = %1 SEQUENCE NO = %2").arg(satid).arg(seqno);
                                }
                                break;

                            //SYSTEM LOG-ON/LOG-OFF
                            case Log_on_request:
                                decodedbytes+="Log_on_request";
                                break;
                            case Log_on_confirm:
                                decline+="Log_on_confirm";
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

                            //CHANNEL INFORMATION
                            case P_R_channel_control_ISU:
                                decline+="P_R_channel_control_ISU";
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

                        decodedbytes+=decline;

                    }

                    //if(!allgood)decodedbytes+="Got at least one bad SU (a SU failed CRC check)\n";//tell the console



                }

            }

            //raw
            //if((cntr-16)<1152)decodedbytes+=('0'+bits[i])+QChar(',');
        }

       // if(cntr+1==1200)cntr=-1;
       // if((framecounter1!=framecounter2||cntr>1300)&&preambledetector.Update(bits[i]))
        if(preambledetector.Update(bits[i]))
        {
            if(cntr+1!=1200)
            {
                isudata.reset();
                decodedbytes+="Error short frame!!! maybe the soundcard dropped some sound card buffers\n";
            }
//            decodedbytes+=((QString)"Bits for frame = %1\n").arg(cntr+1);
            cntr=-1;
//            decodedbytes+="\nGot sync\n";

            //got a signal
            datacd=true;
            datacdcountdown=12;
            emit DataCarrierDetect(datacd);

            scrambler.reset();
        }

        if(cntr+1==1200)
        {
            scrambler.reset();
            cntr=-1;
        }


    }

    if(!datacd)decodedbytes.clear();
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
        for(int j=0;j<8;j++)
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

