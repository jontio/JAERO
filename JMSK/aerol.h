#ifndef AEROL_H
#define AEROL_H

#include <QObject>
#include <QPointer>
#include <QVector>
#include <QList>
#include <QDebug>
#include <assert.h>
#include "../viterbi-xukmin/viterbi.h"

namespace AEROL {
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
};

//defragments 0x71 SUs with its SSUs
class ISUData
{
public:
    ISUData(){missingssu=false;}
    bool update(QByteArray data);
    ISUItem lastvalidisuitem;
    bool missingssu;
private:
    QList<ISUItem> isuitems;
    ISUItem anisuitem;
    int findisuitemC0(ISUItem &anisuitem);
    int findisuitem71(ISUItem &anisuitem);
    void deleteoldisuitems();
};

class ParserISU
{
public:
    ParserISU(){}
    QString toHumanReadableInformation(ISUItem &isuitem);
};

class AeroLcrc16 //this seems to be called GENIBUS not CCITT
{
public:
    AeroLcrc16(){}
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
private:
    QVector<int> matrix;
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

class AeroL : public QIODevice
{
    Q_OBJECT
public:

    explicit AeroL(QObject *parent = 0);
    ~AeroL();
    void ConnectSinkDevice(QIODevice *device);
    void DisconnectSinkDevice();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
signals:
    void HumanReadableInformation(QString str);
public slots:
    void setBitRate(double fb);
private:
    bool Start();
    void Stop();
    QByteArray &Decode(QVector<short> &bits);
    QPointer<QIODevice> psinkdevice;
    QVector<short> sbits;
    QByteArray decodedbytes;
    PreambleDetector preambledetector;

    QVector<int> block;
    AeroLInterleaver leaver;
    AeroLScrambler scrambler;

    ViterbiCodec *convolcodec;
    DelayLine dl1,dl2;

    AeroLcrc16 crc16;

    quint16 frameinfo;
    quint16 lastframeinfo;

    QByteArray infofield;

    ISUData isudata;
    ParserISU parserisu;

};

#endif // AEROL_H
