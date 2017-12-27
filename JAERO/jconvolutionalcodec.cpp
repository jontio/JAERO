#include "jconvolutionalcodec.h"

#include <QDebug>
#include <assert.h>
#include <math.h>
#include <iostream>

JConvolutionalCodec::JConvolutionalCodec(QObject *parent) : QObject(parent)
{  
    paddinglength=24*4;

    correct_convolutional_polynomial_t poly[2];
    poly[0]=109;
    poly[1]=79;
    convol = correct_convolutional_create(2, 7, poly);
    constraint=7;
    nparitybits=2;
}

void JConvolutionalCodec::SetCode(int inv_rate, int order,const QVector<quint16>poly,int _paddinglength)
{
    correct_convolutional_destroy(convol);
    convol = correct_convolutional_create(inv_rate, order, poly.data());
    constraint=order;
    nparitybits=inv_rate;
    soft_bits_overlap_buffer_uchar.clear();
    paddinglength=_paddinglength;

}

JConvolutionalCodec::~JConvolutionalCodec()
{
    correct_convolutional_destroy(convol);
}

QVector<int> &JConvolutionalCodec::Decode_hard(const QByteArray& soft_bits_in, int size)//0-->-1 128-->0 255-->1
{

    soft_bits_overlap_buffer_uchar.clear();

    //pack into bytes
    QByteArray hard_bytes_in;
    uchar uchar_cnt=0;
    uchar hard_byte=0;
    int bit_counter=0;
    for(int i=0;i<size||uchar_cnt;i++)
    {
        hard_byte<<=1;
        if(i<soft_bits_in.size())
        {
            if(((uchar)(soft_bits_in.at(i)))>0)hard_byte|=1;
            bit_counter++;
        }
        uchar_cnt++;
        if(uchar_cnt>=8)
        {
            uchar_cnt=0;
            hard_bytes_in.push_back(hard_byte);
            hard_byte=0;
        }
    }

    soft_bits_overlap_buffer_uchar.append(hard_bytes_in);


    //decoce
    decoded.resize(hard_bytes_in.size()/nparitybits);
    size_t dbits=correct_convolutional_decode(convol,(uchar*)soft_bits_overlap_buffer_uchar.data(),bit_counter,(uchar*)decoded.data());
    assert(dbits>0);
    dbits=bit_counter/nparitybits;

    //unpack bytes
    decoded_bits.resize(dbits);
    int bit_ptr=0;
    for(int i=0;i<decoded.size()&&bit_ptr<((int)dbits);i++)
    {
        uchar uch=decoded[i];
        for(int k=0;k<8;k++)
        {
            if(uch&128)decoded_bits[bit_ptr]=1;
             else decoded_bits[bit_ptr]=0;
            uch<<=1;
            bit_ptr++;
        }
    }

    return decoded_bits;

}
QVector<int> &JConvolutionalCodec::Decode_soft(QByteArray& soft_bits_in, int size)//0-->-1 128-->0 255-->1
{

    soft_bits_overlap_buffer_uchar.clear();
    soft_bits_overlap_buffer_uchar.append(soft_bits_in);

    //decode
    decoded.resize(size/nparitybits);
    size_t dbits=correct_convolutional_decode_soft(convol,(uchar*)soft_bits_overlap_buffer_uchar.data(),size,(uchar*)decoded.data());
    assert(dbits>0);
    dbits=size/nparitybits;

    //unpack bytes
    decoded_bits.resize(dbits);
    int bit_ptr=0;
    for(int i=0;i<decoded.size()&&bit_ptr<((int)dbits);i++)
    {
        uchar uch=decoded[i];
        for(int k=0;k<8;k++)
        {
            if(uch&128)decoded_bits[bit_ptr]=1;
             else decoded_bits[bit_ptr]=0;
            uch<<=1;
            bit_ptr++;
        }
    }

    return decoded_bits;

}


QVector<int> &JConvolutionalCodec::Soft_To_Hard_Convert(const QByteArray& soft_bits_in)
{
    decoded_bits.clear();
    for(int i=0;i<soft_bits_in.size();i++)
    {
        if(((uchar)(soft_bits_in.at(i)))>=128)decoded_bits.push_back(1);
         else decoded_bits.push_back(0);
    }
    return decoded_bits;
}

QByteArray& JConvolutionalCodec::Hard_To_Soft_Convert(QByteArray& hard_bits_in)
{

    for(int i=0;i<hard_bits_in.size();i++)
    {
        if(((uchar)(hard_bits_in.at(i)))==0){
            hard_bits_in[i] = uchar(0);
        }
        else{
            hard_bits_in[i] = uchar(255);

        }
    }
    return hard_bits_in;
}

QVector<int> &JConvolutionalCodec::Decode_Continuous(QByteArray& soft_bits_in)//0-->-1 128-->0 255-->1
{
    int k=62;// polys.size()*(paddinglength+constraint_));

    soft_bits_overlap_buffer_uchar.append(soft_bits_in);

    // add some padding on the back
    QByteArray filldata;
    filldata.fill(128,paddinglength);
    soft_bits_overlap_buffer_uchar.append(filldata);


    //decode
    double bt_size = soft_bits_overlap_buffer_uchar.size();
    bt_size=(bt_size/nparitybits)/8+1;

    decoded.resize((soft_bits_overlap_buffer_uchar.size()/nparitybits)+1);

    size_t dbits=correct_convolutional_decode_soft(convol,(uchar*)soft_bits_overlap_buffer_uchar.data(),soft_bits_overlap_buffer_uchar.size(),(uchar*)decoded.data());
    assert(dbits>0);

    dbits=soft_bits_overlap_buffer_uchar.size()/nparitybits;

    //unpack bytes
    decoded_bits.resize(dbits);

    int bit_ptr=0;

    for(int i=0;i<decoded.size()&&bit_ptr<((int)dbits);i++)
    {
        uchar uch=decoded[i];

        for(int k=0;k<8&&bit_ptr<((int)dbits);k++)
        {
            if(uch&128)decoded_bits[bit_ptr]=1;
             else decoded_bits[bit_ptr]=0;
            uch<<=1;
            bit_ptr++;
        }
    }


    //remove the padding bits from the return data
    decoded_bits=decoded_bits.mid(paddinglength+1, soft_bits_in.size()/nparitybits );

    //remove the used data from the padding buffer
    soft_bits_overlap_buffer_uchar=soft_bits_in.right(k);
    soft_bits_overlap_buffer_uchar.resize(k);

    return decoded_bits;
}

//this is a bit of a hack just to compre soft with hard decoding. paddinglength must be a multiple of 8
QVector<int> &JConvolutionalCodec::Decode_Continuous_hard(const QByteArray& soft_bits_in)//0-->-1 128-->0 255-->1
{
    int k=(2*nparitybits*paddinglength)/8;//msg is padded frount and back both times by paddinglength
    if(soft_bits_overlap_buffer_uchar.size()!=k)soft_bits_overlap_buffer_uchar.fill(0,k);

    //harden the bits up and pack into bytes
    QByteArray hard_bytes_in;
    uchar uchar_cnt=0;
    uchar hard_byte=0;
    int bit_counter=0;
    for(int i=0;i<soft_bits_in.size()||uchar_cnt;i++)
    {
        hard_byte<<=1;
        if(i<soft_bits_in.size())
        {
            if(((uchar)(soft_bits_in.at(i)))>0)hard_byte|=1;
            bit_counter++;
        }
        uchar_cnt++;
        if(uchar_cnt>=8)
        {
            uchar_cnt=0;
            hard_bytes_in.push_back(hard_byte);
            hard_byte=0;
        }
    }

    soft_bits_overlap_buffer_uchar.append(hard_bytes_in);

    //decode hard
    decoded.resize((soft_bits_overlap_buffer_uchar.size()/nparitybits)+1);
    size_t dbits=correct_convolutional_decode(convol,(uchar*)soft_bits_overlap_buffer_uchar.data(),bit_counter+k*8,(uchar*)decoded.data());
    assert(dbits>0);
    dbits=(bit_counter+k*8)/nparitybits;

    //unpack bytes
    decoded_bits.resize(dbits);
    int bit_ptr=0;
    for(int i=0;i<decoded.size()&&bit_ptr<((int)dbits);i++)
    {
        uchar uch=decoded[i];
        for(int k=0;k<8;k++)
        {
            if(uch&128)decoded_bits[bit_ptr]=1;
             else decoded_bits[bit_ptr]=0;
            uch<<=1;
            bit_ptr++;
        }
    }

    //remove the padding bits from the return data
    decoded_bits=decoded_bits.mid(paddinglength);//(paddinglength/nparitybits);//-(constraint-1)+1);
    decoded_bits=decoded_bits.mid(0,decoded_bits.size()-paddinglength);

    //remove the used data from the padding buffer
    soft_bits_overlap_buffer_uchar=soft_bits_overlap_buffer_uchar.right(k);


    return decoded_bits;
}

void JConvolutionalCodec::printbits(QByteArray msg,int nbits)
{
    int nbitscnt=0;
    QString tstr;for(int u=0;u<msg.size();u++)
    {
        uchar uch=msg[u];
        for(int w=0;w<8;w++)
        {
            uchar bit=0;
            if(uch&128)bit=1;
            tstr+=QString::number(bit);
            uch<<=1;
            nbitscnt++;
            if(nbits>0&&nbitscnt>=nbits)break;
        }
        if(nbits>0&&nbitscnt>=nbits)break;
    }
    qDebug()<<tstr;
}
