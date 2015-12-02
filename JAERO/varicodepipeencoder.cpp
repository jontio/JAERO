#include "varicodepipeencoder.h"

#include <QDebug>

VariCodePipeEncoder::VariCodePipeEncoder(QObject *parent) : QIODevice(parent)
{
    encodedbytes.reserve(1000);
    sbits.reserve(1000);
    bc.SetInNumberOfBits(1);
    bc.SetOutNumberOfBits(8);
}

VariCodePipeEncoder::~VariCodePipeEncoder()
{

}

bool VariCodePipeEncoder::Start()
{
    if(psourcedevice.isNull())return false;
    QIODevice *in=psourcedevice.data();
    in->open(QIODevice::ReadOnly|QIODevice::Unbuffered);//QIODevice::Unbuffered to stop reading to much. i dont know how to adjust iodevice's buffer size is seems to default to 16000bytes
    return true;
}

void VariCodePipeEncoder::Stop()
{
    if(!psourcedevice.isNull())
    {
        QIODevice *in=psourcedevice.data();
        in->close();
    }
}

void VariCodePipeEncoder::ConnectSourceDevice(QIODevice *device)
{
    if(!device)return;
    psourcedevice=device;
    Start();
}


void VariCodePipeEncoder::DisconnectSourceDevice()
{
    Stop();
    if(!psourcedevice.isNull())psourcedevice.clear();
}


QVector<short> &VariCodePipeEncoder::Encode(QByteArray &bytes)
{
    for(int i=0;i<bytes.size();i++)
    {
        if(bytes[i]<(char)0)bytes[i]=(char)0;//the vericode im using dont do numbers above 127
    }
    sbits.resize((bytes.size()+1)*16);
    sbits.resize(varicode_encode(sbits.data(),bytes.data(),sbits.size(),bytes.size(),1));
    return sbits;
}

qint64 VariCodePipeEncoder::readData(char *data, qint64 maxlen)
{
    int len=0;

    //qDebug()<<maxlen;

    //use remainder bytes if have them
    for(;encodedbytes.size()&&len<maxlen;len++)
    {
        data[len]=encodedbytes[0];
        encodedbytes.remove(0,1);
    }

    if(len>=maxlen)return len;

    if(!psourcedevice.isNull())
    {
        QIODevice *in=psourcedevice.data();
        if(in->isOpen())bytesin=in->read((maxlen-len)+3);//an approximation. may be too much at times but better more than not enough. +3 cos this ensures that we get at least one encoded byte if the source has 3 bytes available
         else return 0;
    }
     else return 0;

    //if nothing from source then send a SYN char
    if(!bytesin.size())
    {
        bytesin.push_back((char)22);//12 bits after encoding
    }

    Encode(bytesin);

    //fill up data with sbits
    for(int i=0;i<sbits.size();i++)
    {
        bc.LoadSymbol(sbits[i]);
        if(bc.DataAvailable)
        {
            bc.GetNextSymbol();
            encodedbytes.push_back(bc.Result);
        }
    }

    //if we didn't get enough bits we must ensure that we return something.
    //so pad with 0
    if(!encodedbytes.size())
    {
        while(!bc.DataAvailable)
        {
            bc.LoadSymbol(0);
            bc.LoadSymbol(0);
        }
        bc.GetNextSymbol();
        encodedbytes.push_back(bc.Result);
    }

    //use the new bytes and save the remainder of them
    for(;encodedbytes.size()&&len<maxlen;len++)
    {
        data[len]=encodedbytes[0];
        encodedbytes.remove(0,1);
    }

    return len;
}


qint64 VariCodePipeEncoder::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}
