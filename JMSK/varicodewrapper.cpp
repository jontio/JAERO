#include "varicodewrapper.h"

VariCodeWrapper::VariCodeWrapper(QObject *parent) : QIODevice(parent)
{
    varicode_decode_init(&varicode_dec_states,1);
    decodedbytes.reserve(1000);
    sbits.reserve(1000);
}

QByteArray &VariCodeWrapper::Decode(QVector<short> &bits)
{
    decodedbytes.resize(bits.size()+1);
    decodedbytes.resize(varicode_decode(&varicode_dec_states,decodedbytes.data(),bits.data(),decodedbytes.size(),bits.size()));//varicode_decode does not work correctly if you say only one output char. is this a bug?
    return decodedbytes;
}

void VariCodeWrapper::DecodeBits(QVector<short> &bits)
{
    emit DecodedBytes(Decode(bits));
}

void VariCodeWrapper::DecodeBytes(const QByteArray &bitsinbytes)
{
    sbits.clear();
    short ashort;
    for(int i=0;i<bitsinbytes.size();i++)
    {
        ashort=bitsinbytes.at(i);
        for(int j=0;j<8;j++)
        {
            sbits.push_back(ashort&1);
            ashort=ashort>>1;
        }
    }
    emit DecodedBytes(Decode(sbits));
}

VariCodeWrapper::~VariCodeWrapper()
{

}

qint64 VariCodeWrapper::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}

qint64 VariCodeWrapper::writeData(const char *data, qint64 len)
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
    emit DecodedBytes(Decode(sbits));
    return len;
}

