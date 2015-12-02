#include "varicodepipedecoder.h"

VariCodePipeDecoder::VariCodePipeDecoder(QObject *parent) : QIODevice(parent)
{
    varicode_decode_init(&varicode_dec_states,1);
    decodedbytes.reserve(1000);
    sbits.reserve(1000);
}

VariCodePipeDecoder::~VariCodePipeDecoder()
{

}

bool VariCodePipeDecoder::Start()
{
    if(psinkdevice.isNull())return false;
    QIODevice *out=psinkdevice.data();
    out->open(QIODevice::WriteOnly);
    return true;
}

void VariCodePipeDecoder::Stop()
{
    if(!psinkdevice.isNull())
    {
        QIODevice *out=psinkdevice.data();
        out->close();
    }
}

void VariCodePipeDecoder::ConnectSinkDevice(QIODevice *device)
{
    if(!device)return;
    psinkdevice=device;
    Start();
}

void VariCodePipeDecoder::DisconnectSinkDevice()
{
    Stop();
    if(!psinkdevice.isNull())psinkdevice.clear();
}

QByteArray &VariCodePipeDecoder::Decode(QVector<short> &bits)
{
    decodedbytes.resize(bits.size()+1);
    decodedbytes.resize(varicode_decode(&varicode_dec_states,decodedbytes.data(),bits.data(),decodedbytes.size(),bits.size()));//varicode_decode does not work correctly if you say only one output char. is this a bug?
    return decodedbytes;
}

qint64 VariCodePipeDecoder::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}


qint64 VariCodePipeDecoder::writeData(const char *data, qint64 len)
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
