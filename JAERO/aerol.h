#ifndef AEROL_H
#define AEROL_H

#include <QObject>
#include <QPointer>
#include <QVector>
#include <QTimer>
#include <QDateTime>
#include <QList>
#include <QDebug>
#include <assert.h>
#include <math.h>
#include <iostream>
#include "../viterbi-xukmin/viterbi.h"
#include "jconvolutionalcodec.h"

#include "databasetext.h"

namespace AEROTypeR {
typedef enum MessageType
{
    General_access_request_telephone=0x20,
    Abbreviated_access_request_telephone=0x23,
    Access_request_data_R_T_channel=0x22,
    Request_for_acknowledgement_R_channel=0x61,
    Acknowledgement_R_channel=0x62,
    Log_On_Off_control_R_channel=0x12,
    Call_progress_R_channel=0x30,
    Log_On_Off_acknowledgement=0x15,
    Log_control_R_channel_ready_for_reassignment=0x17,
    Telephony_acknowledge_R_channel=0x60,
    User_data_ISU_SSU_R_channel=-1,
} MessageType;
}

namespace AEROTypeT {
typedef enum MessageType
{
    Fill_in_signal_unit=0x01,
    User_data_ISU_RLS_T_channel=0x71,
    User_data_ISU_SSU_T_channel=-1,
} MessageType;
}

namespace AEROTypeP {
typedef enum MessageType
{
    Reserved_0=0x00,
    Fill_in_signal_unit=0x01,

    AES_system_table_broadcast_GES_Psmc_and_Rsmc_channels_COMPLETE=0x05,
    AES_system_table_broadcast_GES_beam_support_COMPLETE=0x07,
    AES_system_table_broadcast_index=0x0A,
    AES_system_table_broadcast_satellite_identification_COMPLETE=0x0C,

    //SYSTEM LOG-ON/LOG-OFF
    Log_on_request=0x10,
    Log_on_confirm=0x11,
    Log_control_P_channel_log_off_request=0x12,
    Log_control_P_channel_log_on_reject=0x13,
    Log_control_P_channel_log_on_interrogation=0x14,
    Log_on_log_off_acknowledge_P_channel=0x15,
    Log_control_P_channel_log_on_prompt=0x16,
    Log_control_P_channel_data_channel_reassignment=0x17,

    Reserved_18=0x18,
    Reserved_19=0x19,
    Reserved_26=0x26,

    //CALL INITIATION
    Data_EIRP_table_broadcast_complete_sequence=0x28,

    //CHANNEL INFORMATION
    P_R_channel_control_ISU=0x40,
    T_channel_control_ISU=0x41,

    T_channel_assignment=0x51,

    //ACKNOWLEDGEMENT
    Request_for_acknowledgement_RQA_P_channel=0x61,
    Acknowledge_RACK_TACK_P_channel=0x62,


    User_data_ISU_RLS_P_T_channel=0x71,
    User_data_3_octet_LSDU_RLS_P_channel=0x74,
    User_data_4_octet_LSDU_RLS_P_channel=0x76


} MessageType;

}

struct ISUItem {
    quint32 AESID;
    uchar GESID;
    uchar QNO;
    uchar SEQNO;
    uchar REFNO;
    uchar NOOCTLESTINLASTSSU;
    QByteArray userdata;
    int count;
    void clear()
    {
        AESID=0;
        GESID=0;
        QNO=0;
        SEQNO=0;
        REFNO=0;
        NOOCTLESTINLASTSSU=0;
        userdata.clear();
        count=0;
    }
};


struct RISUItem : ISUItem{
    int SEQINDICATOR;
    int SUTYPE;
    int filledarray;
    void clear()
    {
        ISUItem::clear();
        SEQINDICATOR=0;//for R
        SUTYPE=0;//for R
        filledarray=0;//for R
    }
};

class ACARSItem : public DBase
{
public:
    ISUItem isuitem;

    char MODE;
    uchar TAK;
    QByteArray LABEL;
    uchar BI;
    QByteArray PLANEREG;
    QStringList dblookupresult;
    bool nonacars;

    bool downlink;
    bool valid;
    bool hastext;
    bool moretocome;
    QString message;
    void clear()
    {
        isuitem.clear();
        valid=false;
        hastext=false;
        moretocome=false;
        MODE=0;
        TAK=0;
        BI=0;
        nonacars=false;
        PLANEREG.clear();
        LABEL.clear();
        message.clear();
        downlink=false;
    }
    explicit ACARSItem(){clear();}
};

//defragments 0x71 SUs with its SSUs
class ISUData
{
public:
    ISUData(){missingssu=false;}
    bool update(QByteArray data);
    ISUItem lastvalidisuitem;
    bool missingssu;
    void reset(){isuitems.clear();}
private:
    QList<ISUItem> isuitems;
    ISUItem anisuitem;
    int findisuitemC0(ISUItem &anisuitem);
    int findisuitem71(ISUItem &anisuitem);
    void deleteoldisuitems();
};

//defragments R channel SUs with its SSUs
class RISUData
{
public:
    RISUData(){}
    bool update(QByteArray data);
    RISUItem lastvalidisuitem;
    void reset(){isuitems.clear();}
private:
    QList<RISUItem> isuitems;
    RISUItem anisuitem;
    int findisuitem(RISUItem &anisuitem);
    void deleteoldisuitems();
};


class ACARSDefragmenter
{
public:
    ACARSDefragmenter(){}
    bool defragment(ACARSItem &acarsitem);
private:
    struct ACARSItemext{
        ACARSItem anacarsitem;
        int count;
    };
    QList<ACARSItemext> acarsitemexts;
    ACARSItemext anacarsitemext;
    int findfragment(ACARSItem &acarsitem);
};

class ParserISU : public QObject
{
    Q_OBJECT
public:
    explicit ParserISU(QObject *parent = 0);
    bool parse(ISUItem &isuitem);
    bool downlink;
signals:
    void ACARSsignal(ACARSItem &acarsitem);
    void Errorsignal(QString &error);
public slots:
    void setDataBaseDir(const QString &dir);
private:
    ACARSDefragmenter acarsdefragmenter;
    ACARSItem anacarsitem;
    QString anerror;
    QString databasedir;
    DataBaseTextUser *dbtu;
private slots:
    void acarslookupresult(bool ok, int ref, const QStringList &result);
};

class PuncturedCode
{
public:
    PuncturedCode();
    void depunture_soft_block(QByteArray &block,int pattern, bool reset=true);
    void punture_soft_block(QByteArray &block, int pattern, bool reset=true);
private:
    int punture_ptr;
    int depunture_ptr;
};

class AeroLcrc16 //this seems to be called GENIBUS not CCITT
{
public:
    AeroLcrc16(){}
    bool calcusingbitsandcheck(int *bits,int numberofbits)
    {

        quint16 crc_rec=0;
        for(int i=numberofbits-1;i>=numberofbits-16; i--)
        {
            crc_rec<<=1;
            crc_rec|=bits[i];
        }
        numberofbits-=16;

        quint16 crc = 0xFFFF;
        int crc_bit;
        for(int i=0; i<numberofbits; i++)//we are finished when all bits of the message are looked at
        {
            //crc_bit = (crc >> 15) & 1;//bit of crc we are working on. 15=poly order-1
            //crc <<= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
            //if(crc_bit ^ bits[i])crc = crc ^ 0x1021;//add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

            //differnt endiness
            crc_bit = crc & 1;//bit of crc we are working on. 15=poly order-1
            crc >>= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
            if(crc_bit ^ bits[i])crc = crc ^ 0x8408;//(0x8408 is reversed 0x1021)add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

        }
        crc=~crc;
        if(crc_rec==crc)return true;
        return false;
    }
    quint16 calcusingbits(int *bits,int numberofbits)
    {
        quint16 crc = 0xFFFF;
        int crc_bit;
        for(int i=0; i<numberofbits; i++)//we are finished when all bits of the message are looked at
        {
            //crc_bit = (crc >> 15) & 1;//bit of crc we are working on. 15=poly order-1
            //crc <<= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
            //if(crc_bit ^ bits[i])crc = crc ^ 0x1021;//add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

            //differnt endiness
            crc_bit = crc & 1;//bit of crc we are working on. 15=poly order-1
            crc >>= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
            if(crc_bit ^ bits[i])crc = crc ^ 0x8408;//(0x8408 is reversed 0x1021)add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

        }
        return ~crc;
    }
    quint16 calcusingbytes(char *bytes,int numberofbytes)
    {
        quint16 crc = 0xFFFF;
        int crc_bit;
        int message_bit;
        int message_byte;
        for(int i=0; i<numberofbytes; i++)//we are finished when all bits of the message are looked at
        {
            message_byte=bytes[i];
            for(int k=0;k<8;k++)
            {

                //message_bit=(message_byte>>7)&1;
                //message_byte<<=1;
                //crc_bit = (crc >> 15) & 1;//bit of crc we are working on. 15=poly order-1
                //crc <<= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
                //if(crc_bit ^ message_bit)crc = crc ^ 0x1021;//add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

                //differnt endiness
                message_bit=message_byte&1;
                message_byte>>=1;
                crc_bit = crc & 1;//bit of crc we are working on. 15=poly order-1
                crc >>= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
                if(crc_bit ^ message_bit)crc = crc ^ 0x8408;//(0x8408 is reversed 0x1021)add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

            }
        }
        return ~crc;
    }
    quint16 calcusingbytesotherendines(char *bytes,int numberofbytes)
    {
        quint16 crc = 0xFFFF;
        int crc_bit;
        int message_bit;
        int message_byte;
        for(int i=0; i<numberofbytes; i++)//we are finished when all bits of the message are looked at
        {
            message_byte=bytes[i];
            for(int k=0;k<8;k++)
            {

                message_bit=(message_byte>>7)&1;
                message_byte<<=1;
                crc_bit = (crc >> 15) & 1;//bit of crc we are working on. 15=poly order-1
                crc <<= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
                if(crc_bit ^ message_bit)crc = crc ^ 0x1021;//add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

                //differnt endiness
                //message_bit=message_byte&1;
                //message_byte>>=1;
                //crc_bit = crc & 1;//bit of crc we are working on. 15=poly order-1
                //crc >>= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
                //if(crc_bit ^ message_bit)crc = crc ^ 0x8408;//(0x8408 is reversed 0x1021)add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)

            }
        }
        return ~crc;
    }
};

class AeroLScrambler
{
public:
    AeroLScrambler()
    {
        reset();
    }
    void reset()
    {
        int tmp[]={1,1,0,1,0,0,1,0,1,0,1,1,0,0,1,-1};
        state.clear();
        for(int i=0;tmp[i]>=0;i++)state.push_back(tmp[i]);
    }
    void update(QVector<int> &data)
    {
        for(int j=0;j<data.size();j++)
        {
            int val0=state[0]^state[14];
            data[j]^=val0;
            for(int i=state.size()-1;i>0;i--)state[i]=state[i-1];
            state[0]=val0;
        }
    }
private:
    QVector<int> state;
};

class DelayLine
{
public:
    DelayLine()
    {
        setLength(12);
    }
    void setLength(int length)
    {
        length++;
        assert(length>0);
        buffer.resize(length);
        buffer_ptr=0;
        buffer_sz=buffer.size();
    }
    void update(QVector<int> &data)
    {
        for(int i=0;i<data.size();i++)
        {
            buffer[buffer_ptr]=data[i];
            buffer_ptr++;buffer_ptr%=buffer_sz;
            data[i]=buffer[buffer_ptr];
        }
    }
private:
    QVector<int> buffer;
    int buffer_ptr;
    int buffer_sz;
};

class AeroLInterleaver
{
public:
    AeroLInterleaver();
    void setSize(int N);
    QVector<int> &interleave(QVector<int> &block);
    QVector<int> &deinterleave(QVector<int> &block);
    QVector<int> &deinterleaveMSK(QVector<int> &block, int blocks);
    QByteArray &deinterleaveMSK_ba(QVector<int> &block, int blocks);
    QByteArray &deinterleave_ba(QVector<int> &block, int blocks);


    QVector<int> &deinterleave(QVector<int> &block,int cols);//deinterleaves with a fewer number of cols than the block has
private:
    QVector<int> matrix;
    QByteArray matrix_ba;
    int M;
    int N;
    QVector<int> interleaverowpermute;
    QVector<int> interleaverowdepermute;
};

class PreambleDetector
{
public:
    PreambleDetector();
    void setPreamble(QVector<int> _preamble);
    bool setPreamble(quint64 bitpreamble,int len);
    bool Update(int val);
private:
    QVector<int> preamble;
    QVector<int> buffer;
    int buffer_ptr;
};

class PreambleDetectorPhaseInvariant
{
public:
    PreambleDetectorPhaseInvariant();
    void setPreamble(QVector<int> _preamble);
    bool setPreamble(quint64 bitpreamble,int len);
    void setTollerence(int tollerence);
    int Update(int val);
    bool inverted;
private:
    QVector<int> preamble;
    QVector<int> buffer;
    int buffer_ptr;
    int tollerence;
};

class OQPSKPreambleDetectorAndAmbiguityCorrection
{
public:
    OQPSKPreambleDetectorAndAmbiguityCorrection();
    void setPreamble(QVector<int> _preamble);
    bool setPreamble(quint64 bitpreamble,int len);
    bool Update(int val);
private:
    QVector<int> preamble;
    QVector<int> buffer;
    int buffer_ptr;
};

class RTChannelDeleaveFECScram
{
public:

    int targetSUSize =0;
    int targetBlocks = 0;

    typedef enum ReturnResult
    {
        OK_Packet=      0b00000001,
        OK_R_Packet=    0b00000011,
        OK_T_Packet=    0b00000101,
        Bad_Packet=     0b00000000,
        Test_Failed=    0b00100000,
        Nothing=        0b00001000,
        FULL=           0b00010000
    } ReturnResult;
    RTChannelDeleaveFECScram()
    {
        //ViterbiCodec is not Qt so needs deleting when finished
        std::vector<int> polynomials;
        polynomials.push_back(109);
        polynomials.push_back(79);
        convolcodec=new ViterbiCodec(7, polynomials);
        convolcodec->setPaddingLength(24);

        block.resize(64*95);//max rows*cols
        leaver.setSize(95);//max cols needed

        lastpacketstate=Nothing;

        //jeroen

        jconvolcodec = new JConvolutionalCodec(0);
        QVector<quint16> polys;
        polys.push_back(109);
        polys.push_back(79);
        jconvolcodec->SetCode(2,7,polys,0);



        resetblockptr();
    }
    ~RTChannelDeleaveFECScram()
    {
        delete convolcodec;
        delete jconvolcodec;
    }
    ReturnResult resetblockptr()
    {
        blockptr=0;
        if(lastpacketstate==Test_Failed)
        {
            lastpacketstate=Nothing;
            return Bad_Packet;
        }
        lastpacketstate=Nothing;
        return Nothing;
    }
    void packintobytes()
    {
        infofield.clear();
        int charptr=0;
        uchar ch=0;
        //int ctt=0;
        //QString str2;
        for(int h=0;h<deconvol.size();h++)//take one for flushing
        {
            ch|=deconvol[h]*128;
            charptr++;charptr%=8;
            if(charptr==0)
            {
                infofield+=ch;

                // ctt++;
                // str2+=((QString)"0x%1 ").arg(((QString)"").sprintf("%02X", ch));
                // if((ctt-6)%12==0)str2+="\n";

                ch=0;
            }
            else ch>>=1;
        }

        //qDebug()<<str2.toLatin1().data();
    }


    ReturnResult updateMSK(int bit)
    {
        if(blockptr>=block.size())return FULL;
        block[blockptr]=bit;
        blockptr++;


        int ok = 0;

        if(blockptr>=block.size())qDebug()<<"FULL";

        // for an R Packet we need 5 blocks. For a T Packet check at 8 blocks to find total number
        // of SU's
        bool cont = false;


        if((((blockptr-(64*5))%(64*3))==0) && (blockptr /64 == 5 || blockptr /64 == targetBlocks || blockptr/64 == 8 || blockptr/64 == 50)){

            cont = true;

        }


        //test if interleaver length works
        if(cont)//true for R and T packets
        {


            //reset scrambler
            scrambler.reset();

            //deinterleaver
            delBlock = leaver.deinterleaveMSK_ba(block, blockptr/64);

            //decode
            deconvol=jconvolcodec->Decode_soft(delBlock, blockptr);

            //scrambler
            scrambler.update(deconvol);

            //test for R or packet
            if(blockptr==(64*5))
            {
                targetSUSize =0;
                targetBlocks =0;


                //test crc
                bool crcok=crc16.calcusingbitsandcheck(deconvol.data(),8*19);
                if(crcok)
                {

                    //pack into bytes
                    packintobytes();

                    blockptr=block.size();//stop further testing
                    lastpacketstate=OK_R_Packet;
                    return OK_R_Packet;
                }
            }

            //Test for T packet
            //test header crc
            bool crcok=crc16.calcusingbitsandcheck(deconvol.data(),8*6);
            if(!crcok)
            {

                if(blockptr>=block.size())
                {
                    lastpacketstate=Bad_Packet;
                    return Bad_Packet;
                }

                lastpacketstate=Bad_Packet;
                return Bad_Packet;
            }
            // good T packet 48 bit header. If we do not yet know how many SU's, go and check this if block nr is 8
            else
            {

                if(blockptr/64 ==5){
                    return Nothing;
                }

                if(blockptr/64 == 8){

                    // we should be able to peek at the SU after the initial SU and figure out the number of SU's in this
                    // burst

                    QVector<int> isu = deconvol.mid((8*6)+(8*12)*1, 8*12);
                    for(int a = 0; a < 6 ; a++){

                    }

                    int bin = 2;
                    bin+= ((isu[0] * 1) + (isu[1] * 2) + (isu[2] * 4) + (isu[3] * 8) + (isu[4] * 16) + (isu[5] * 32));

                    targetSUSize = bin;

                    std::cout << "target su " << bin << "\r\n" << std::flush;

                    if(targetSUSize >= 16){
                        targetSUSize = floor(targetSUSize/2) +1;

                    }

                    targetBlocks = (targetSUSize*3) +2;

                    return Nothing;
                }

                // this should be the target blocks for this T packet
                if(blockptr/64 == targetBlocks)
                {
                    for(int i=0;i<=targetSUSize;i++)
                    {
                        crcok=crc16.calcusingbitsandcheck(deconvol.data()+(8*6)+(8*12)*i,8*12);

                        if(crcok){

                            ok++;


                            }// end crc ok

                    }// end SU loop

                    if(ok<=targetSUSize){

                         std::cout << " \r\n num ok " << ok << " target size " << targetSUSize << "\r\n" << std::flush;

                         //pack into bytes
                        packintobytes();
                        infofield.chop(1);
                        numberofsus = targetSUSize;
                        blockptr=block.size();//stop further testing
                        lastpacketstate=OK_T_Packet;

                        return OK_T_Packet;


                    }
                }// end target block loop
                // Max Blocks in one loop

                return Nothing;

            }

            //pack into bytes
            packintobytes();
            infofield.chop(1);


            blockptr=block.size();//stop further testing
            lastpacketstate=OK_T_Packet;
            return OK_T_Packet;

        }
        return Nothing;
    }


    ReturnResult update(int bit)
    {
        if(blockptr>=block.size())return FULL;
        block[blockptr]=bit;
        blockptr++;


        //test if interleaver length works
        if(((blockptr-(64*5))%(64*3))==0)//true for R and T packets
        {

            //reset scrambler
            scrambler.reset();

            //deinterleaver
            //deleaveredblock=leaver.deinterleave(block,blockptr/64);
            delBlock = leaver.deinterleave_ba(block, blockptr/64);


            //decode
            //deconvol=convolcodec->Decode(deleaveredblock,blockptr);
            deconvol=jconvolcodec->Decode_soft(delBlock, blockptr);

            //scrambler
            scrambler.update(deconvol);

            //test for R packet
            if(blockptr==(64*5))
            {
                //test crc
                bool crcok=crc16.calcusingbitsandcheck(deconvol.data(),8*19);
                if(!crcok)
                {
                    lastpacketstate=Test_Failed;
                    return Test_Failed;
                }

                //pack into bytes
                packintobytes();

                //qDebug()<<"CRC OK R packet";

                blockptr=block.size();//stop further testing
                lastpacketstate=OK_R_Packet;
                return OK_R_Packet;
            }

            //Test for T packet
            //test header crc
            bool crcok=crc16.calcusingbitsandcheck(deconvol.data(),8*6);
            if(!crcok)
            {
                if(blockptr>=block.size())
                {
                    lastpacketstate=Bad_Packet;
                    return Bad_Packet;
                }
                lastpacketstate=Test_Failed;
                return Test_Failed;
            }

            //test all the SU crcs
            numberofsus=1+(blockptr-(64*5))/(64*3);
            for(int i=0;i<numberofsus;i++)
            {
                crcok=crc16.calcusingbitsandcheck(deconvol.data()+(8*6)+(8*12)*i,8*12);
                if(!crcok)
                {
                    if(blockptr>=block.size())
                    {
                        lastpacketstate=Bad_Packet;
                        return Bad_Packet;
                    }
                    lastpacketstate=Test_Failed;
                    return Test_Failed;
                }


            }

            //pack into bytes
            packintobytes();
            infofield.chop(1);

            //qDebug()<<"CRC OK T packet with"<<numberofsus<<"SUs\n";

            blockptr=block.size();//stop further testing
            lastpacketstate=OK_T_Packet;
            return OK_T_Packet;

        }
        return Nothing;
    }



    QVector<int> deleaveredblock;
    QByteArray delBlock;
    QVector<int> deconvol;
    QVector<int> block;
    int blockptr;
    AeroLInterleaver leaver;
    AeroLScrambler scrambler;
    ViterbiCodec *convolcodec;
    JConvolutionalCodec *jconvolcodec;
    QByteArray infofield;
    AeroLcrc16 crc16;
    ReturnResult lastpacketstate;
    int numberofsus;
};

class AeroL : public QIODevice
{
    Q_OBJECT
public:
    enum ChannelType {PChannel,RChannel,TChannel};

    explicit AeroL(QObject *parent = 0);
    ~AeroL();
    void ConnectSinkDevice(QIODevice *device);
    void DisconnectSinkDevice();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
signals:
    void DataCarrierDetect(bool status);
    void ACARSsignal(ACARSItem &acarsitem);
    void Errorsignal(QString &error);
public slots:
    void setBitRate(double fb);
    void setBurstmode(bool burstmode);
    void setSettings(double fb, bool burstmode);
    void SignalStatusSlot(bool signal)
    {
        if(!signal)LostSignal();
    }
    void LostSignal()
    {
        cntr=1000000000;
        datacdcountdown=0;
        datacd=false;
        emit DataCarrierDetect(false);
    }
    void setDoNotDisplaySUs(QVector<int> &list){donotdisplaysus=list;}
    void setDataBaseDir(const QString &dir){parserisu->setDataBaseDir(dir);}
    void processDemodulatedSoftBits(const QVector<short> &soft_bits);
private:
    bool Start();
    void Stop();
    QByteArray &Decode(QVector<short> &bits, bool soft = false);
    QPointer<QIODevice> psinkdevice;
    QVector<short> sbits;
    QByteArray decodedbytes;
    PreambleDetector preambledetector;

    //burstmode really not sure this is used
    PreambleDetector preambledetectorburst;

    // 600 / 1200 MSK phase invariant burst detector
    PreambleDetectorPhaseInvariant mskBurstDetector;

    int muw;
    bool burstmode;
    int ifb;
    RTChannelDeleaveFECScram rtchanneldeleavefecscram;
    //

    //OQPSK
    PreambleDetectorPhaseInvariant preambledetectorphaseinvariantimag;
    PreambleDetectorPhaseInvariant preambledetectorphaseinvariantreal;

    bool useingOQPSK;
    int AERO_SPEC_NumberOfBits;//info only
    int AERO_SPEC_TotalNumberOfBits;//info and header and uw
    int AERO_SPEC_BitsInHeader;
    int realimag;



    QVector<int> block;
    AeroLInterleaver leaver;
    AeroLScrambler scrambler;

    ViterbiCodec *convolcodec;
    JConvolutionalCodec * jconvolcodec;

    DelayLine dl1,dl2;

    AeroLcrc16 crc16;

    quint16 frameinfo;
    quint16 lastframeinfo;

    QByteArray infofield;


    RISUData risudata;
    ISUData isudata;
    ParserISU *parserisu;

    int datacdcountdown;
    bool datacd;
    int cntr;

    QVector<int> donotdisplaysus;

    //old static
    //static int cntr=1000000000;
    int formatid;
    int supfrmaker;
    int framecounter1;
    int framecounter2;
    int gotsync_last;
    int blockcnt;




private slots:
    void updateDCD();

};

#endif // AEROL_H
