#include "aerol.h"

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

    qDebug()<<preamble;
    return true;
}
bool PreambleDetector::Update(int val)
{ 
    for(int i=0;i<(buffer.size()-1);i++)buffer[i]=buffer[i+1];
    buffer[buffer.size()-1]=val;
    if(buffer==preamble){qDebug()<<"PreambleDetector: found";return true;}
    return false;
}

AeroL::AeroL(QObject *parent) : QIODevice(parent)
{
    sbits.reserve(1000);
    decodedbytes.reserve(1000);
    preambledetector.setPreamble(3780831379LL,32);//0x3780831379,0b11100001010110101110100010010011
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

QByteArray &AeroL::Decode(QVector<short> &bits)//0 --> oldest
{
    decodedbytes.clear();

    //for(int i=(bits.size()-1);i>=0;i--)
    //for(int i=0;i<bits.size();i++)decodedbytes.push_back((bits[i])+48);
    //return decodedbytes;

    for(int i=0;i<bits.size();i++)
    {
        if(preambledetector.Update(bits[i]))decodedbytes="Got sync\n";
    }
    //if(decodedbytes.isEmpty())decodedbytes=".";

   // qDebug()<<decodedbytes;
   // static int ii=0;
   // ii++;ii%=10;
   // if(ii!=0)decodedbytes.clear();

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

