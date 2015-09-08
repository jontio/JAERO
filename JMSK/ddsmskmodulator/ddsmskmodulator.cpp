#include "ddsmskmodulator.h"
#include <QtEndian>
#include <algorithm>
#include <QDebug>

DDSMSKModulator::DDSMSKModulator(QObject *parent)
:   QIODevice(parent)
{
    slip = new Slip(this);
    JDDSTimeout = new QTimer(this);
    connect(slip,SIGNAL(rxpacket(QByteArray)),this,SLOT(handlerxslippacket(QByteArray)));
    connect(JDDSTimeout,SIGNAL(timeout()),this,SLOT(TimeoutSlot()));
}

void DDSMSKModulator::TimeoutSlot()
{
    WarningMsg("No responce from JDDS device");
    JDDSTimeout->stop();
    stop();

}

void DDSMSKModulator::stoptimeout()
{
    JDDSTimeout->stop();
}

void DDSMSKModulator::setSettings(Settings _settings)
{
    stop();
    bool change=false;
    if(settings.fb!=_settings.fb)change=true;
    if(settings.freq_center!=_settings.freq_center)change=true;
    settings=_settings;
    if(settings.serialportname=="None"||settings.serialportname.isEmpty())
    {
        slip->close();
        return;
    }
    if(slip->getPortName()!=settings.serialportname||!slip->isOpen())
    {
        slip->close();
        slip->setPortName(settings.serialportname);
        slip->setBaudRate(QSerialPort::Baud19200);
        if(!slip->open(QIODevice::ReadWrite))WarningMsg("Cant open serial port \""+settings.serialportname+"\"");
        change=true;
    }
    if(change)
    {
        setJDDSstate(false);
        setJDDSSymbolRate(settings.fb);

        QList<float> freqs;
        float freq=settings.freq_center-settings.fb/4.0;
        freqs.push_back(freq);
        freq=settings.freq_center+settings.fb/4.0;
        freqs.push_back(freq);
        setJDDSfreqs(freqs);

    }
}

///Connects a source device to the modem for the data to modulate
void DDSMSKModulator::ConnectSourceDevice(QIODevice *_datasourcedevice)
{
    if(!_datasourcedevice)return;
    pdatasourcedevice=_datasourcedevice;
    if(pdatasourcedevice.isNull())return;
    QIODevice *io=pdatasourcedevice.data();
    io->open(QIODevice::ReadOnly|QIODevice::Unbuffered);//!!!overrides the settings //QIODevice::Unbuffered to stop reading to much. i dont know how to adjust iodevice's buffer size is seems to default to 16000bytes
}

///Disconnects the source device from the modem
void DDSMSKModulator::DisconnectSourceDevice()
{
    if(pdatasourcedevice.isNull())return;
    QIODevice *io=pdatasourcedevice.data();
    io->close();
    pdatasourcedevice.clear();
}

void DDSMSKModulator::start()
{
    bitstosendbeforereadystatesignal=settings.fb*settings.secondsbeforereadysignalemited;
    bitcounter=0;
    if(!(slip->isOpen()))
    {
        stop();
        if(settings.serialportname=="None"||settings.serialportname.isEmpty())WarningMsg("No serial port set for JDD device");
        setSettings(settings);
        if(!(slip->isOpen()))return;
    }
    emit ReadyState(false);
    open(QIODevice::ReadOnly);

    //queue 2 secs of buffer before activating
    reqbuffer.clear();
    int twosecbuffersize=2.0*settings.fb/8.0;//two secs of buffer
    while(twosecbuffersize)
    {
        int buffersizetoload=twosecbuffersize;
        if(buffersizetoload>254)
        {
            buffersizetoload=254;
        }

        reqbuffer.resize(buffersizetoload);
        reqbuffer.resize(readData(reqbuffer.data(),buffersizetoload));
        if(!reqbuffer.size())break;
        queueJDDSData(reqbuffer);

        twosecbuffersize-=buffersizetoload;
    }

    setJDDSstate(true);
    emit opened();
    emit statechanged(true);
}

void DDSMSKModulator::stop()
{
    if(slip->isOpen())setJDDSstate(false);
    close();
    emit closed();
    emit statechanged(false);
}

void DDSMSKModulator::stopgraceful()
{
    JDDSTimeout->start(6000);
    ignorereq=true;
}

void DDSMSKModulator::handlerxslippacket(QByteArray pkt)
{
    JDDSTimeout->stop();
    uchar pkt_type=pkt[0];
    pkt.remove(0,1);
    switch(pkt_type)//switch on packet type
    {
    case JPKT_GEN_ACK:
        if(pkt.length()>=2)SlipMsg(((QString)"JPKT_GEN_ACK %1 %2").arg((uchar)pkt.at(0)).arg((uchar)pkt.at(1)));
         else if(pkt.length()>=1)SlipMsg(((QString)"JPKT_GEN_ACK %1").arg((uchar)pkt.at(0)));
          else if(pkt.length()>=0)SlipMsg(((QString)"JPKT_GEN_ACK"));
        break;
    case JPKT_GEN_NAK:
        if(pkt.length()>=2)SlipMsg(((QString)"JPKT_GEN_NAK %1 %2").arg((uchar)pkt.at(0)).arg((uchar)pkt.at(1)));
         else if(pkt.length()>=1)SlipMsg(((QString)"JPKT_GEN_NAK %1").arg((uchar)pkt.at(0)));
          else if(pkt.length()>=0)SlipMsg(((QString)"JPKT_GEN_NAK"));
        break;
    case JPKT_SET_FREQS:
        if(!(pkt.length()%4))
        {
            SlipMsg(((QString)"JPKT_SET_FREQS"));
        }
        break;
    case JPKT_FREQ:
        if(pkt.length()==(4))
        {
            qint32 freq=qFromLittleEndian<qint32>((uchar*)pkt.data());
            SlipMsg(((QString)"Freq on DDS is %1").arg(freq));
        }
        break;
    case JPKT_PHASE:
        if(pkt.length()==(4))
        {
            qint32 phase=qFromLittleEndian<qint32>((uchar*)pkt.data());
            SlipMsg(((QString)"Phase on DDS is %1").arg(phase));
        }
        break;
    case JPKT_ON:
        if(pkt.length()==(1))
        {
            if(pkt[0])
            {

                emit opened();
                emit statechanged(true);

                SlipMsg("DDS is on");
            }
            else
            {

                ignorereq=false;
                close();
                emit closed();
                emit statechanged(false);

                if(isOpen())
                {
                    SlipMsg("DDS is off. Device not closed");
                } else SlipMsg("DDS is off");
            }
        }
        break;
    case JPKT_REQ:
        if(pkt.length()==(1)&&!ignorereq)
        {
            uchar byteswanted=pkt[0]-1;
            //SlipMsg(((QString)"Arduino wants %1 more bytes").arg(byteswanted));

            reqbuffer.resize(byteswanted);
            reqbuffer.resize(readData(reqbuffer.data(),byteswanted));
            queueJDDSData(reqbuffer);

        }else JDDSTimeout->start(5000);
        break;
    case JPKT_DATA_ACK:
        //if(!pkt.length())SlipMsg("ACK of data sent to Arduino");
        break;
    case JPKT_DATA_NACK:
        if(!pkt.length())SlipMsg("NACK of data sent to Arduino");
        break;
    case JPKT_PILOT_ON:
        if(!pkt.length())SlipMsg("DDS Pilot on");
        break;
    case JPKT_SYMBOL_PERIOD:
        if(pkt.length()==(4))
        {
            qint32 symbol_period=qFromLittleEndian<qint32>((uchar*)pkt.data());
            SlipMsg(((QString)"Symbol rate on DDS is %1").arg(1000000/((double)symbol_period)));
        }
        break;
    }

}

void DDSMSKModulator::queueJDDSData(QByteArray &data)
{
    if(data.isEmpty())return;
    ba.clear();
    ba.push_back((char)JPKT_Data);
    ba.push_back(data);
    slip->sendpacket(ba);
}

void DDSMSKModulator::setJDDSstate(bool state)
{
    ignorereq=false;
    if(!slip->isOpen())return;
    JDDSTimeout->start(1000);
    ba.clear();
    ba.push_back((char)JPKT_ON);
    if(state)ba.push_back((char)0x02);
     else ba.push_back((char)0x00);
    slip->sendpacket(ba);
    buffer.clear();
}

void DDSMSKModulator::setJDDSfreqs(QList<float> freqs)
{
    if(!slip->isOpen())return;
    JDDSTimeout->start(1000);
    //set frequencys of dds
    quint32 value;
    ba.clear();
    ba.push_back((char)JPKT_SET_FREQS);
    for(int i=0;i<freqs.length();i++)
    {
        value=*(reinterpret_cast<quint32*>(&freqs[i]));
        ba.push_back((char)((value>>(8*0))&0xFF));
        ba.push_back((char)((value>>(8*1))&0xFF));
        ba.push_back((char)((value>>(8*2))&0xFF));
        ba.push_back((char)((value>>(8*3))&0xFF));
    }
    slip->sendpacket(ba);
}

void DDSMSKModulator::setJDDSSymbolRate(double rate)
{
    if(!slip->isOpen())return;
    JDDSTimeout->start(1000);
    //set symbol rate of Arduino
    quint32 value=1000000/rate;
    ba.clear();
    ba.push_back((char)JPKT_SYMBOL_PERIOD);
    ba.push_back((char)((value>>(8*0))&0xFF));
    ba.push_back((char)((value>>(8*1))&0xFF));
    ba.push_back((char)((value>>(8*2))&0xFF));
    ba.push_back((char)((value>>(8*3))&0xFF));
    slip->sendpacket(ba);
}

qint64 DDSMSKModulator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}


qint64 DDSMSKModulator::readData(char *data, qint64 maxlen)
{

    //ensures that exactly maxlen bytes are returned. this keeps the calls from the dds at a fixed rate
    QIODevice *io=pdatasourcedevice.data();
    if(!io->isOpen())return 0;
    int len=std::min(buffer.size(),(int)maxlen);
    while(len<maxlen)
    {
        ba.resize(maxlen-len);
        ba.resize(io->read(ba.data(),maxlen-len));
        if(!ba.size())break;
        buffer.push_back(ba);
        len+=ba.size();
    }
    for(int i=0;i<len;i++)
    {
        data[i]=buffer[i];
    }
    buffer.remove(0,len);

    //emits a signal when a certin number of bits have been sent. can be used for various tasks
    if(bitcounter<bitstosendbeforereadystatesignal)
    {
        bitcounter+=8*len;
        if(bitcounter>=bitstosendbeforereadystatesignal)emit ReadyState(true);
    }

    return len;
}


