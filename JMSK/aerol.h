#ifndef AEROL_H
#define AEROL_H

#include <QObject>
#include <QPointer>
#include <QVector>
#include <QDebug>
#include <assert.h>
#include "../viterbi-xukmin/viterbi.h"

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
            crc_bit = (crc >> 15) & 1;//bit of crc we are working on. 15=poly order-1
            crc <<= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
            if(crc_bit ^ bits[i])crc = crc ^ 0x1021;//add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)
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
                message_bit=(message_byte>>7)&1;
                message_byte<<=1;
                crc_bit = (crc >> 15) & 1;//bit of crc we are working on. 15=poly order-1
                crc <<= 1;//shift to next crc bit (have to do this here before Gate A does its thing)
                if(crc_bit ^ message_bit)crc = crc ^ 0x1021;//add to the crc the poly mod2 if crc_bit + block_bit = 1 mod2 (0x1021 is the ploy with the first bit missing so this means x^16+x^12+x^5+1)
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

public slots:
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
    DelayLine dl1,dl2,dl3;

    AeroLcrc16 crc16;

    quint16 frameinfo;
    quint16 lastframeinfo;

    QByteArray infofield;

};

#endif // AEROL_H
