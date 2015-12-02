#include "mskmodulator.h"
#include <QDebug>

MskModulator::MskModulator(QObject *parent)
:   QIODevice(parent)
{
    osc.SetFreq(settings.freq_center,settings.Fs);
    st.SetFreq(settings.fb,settings.Fs);
    bc.SetInNumberOfBits(8);
    bc.SetOutNumberOfBits(1);
}

void MskModulator::setSettings(Settings _settings)
{
    settings=_settings;
    if((settings.freq_center+(1.5/2.0)*settings.fb)>(settings.Fs/2.0))settings.freq_center=(settings.Fs/2.0)-((1.5/2.0)*settings.fb);
    if((settings.freq_center-(1.5/2.0)*settings.fb)<0.0)settings.freq_center=((1.5/2.0)*settings.fb);
    osc.SetFreq(settings.freq_center,settings.Fs);
    st.SetFreq(settings.fb,settings.Fs);
}

MskModulator::~MskModulator()
{

}

///Connects a source device to the modem for the data to modulate
void MskModulator::ConnectSourceDevice(QIODevice *_datasourcedevice)
{
    if(!_datasourcedevice)return;
    pdatasourcedevice=_datasourcedevice;
    if(pdatasourcedevice.isNull())return;
    QIODevice *io=pdatasourcedevice.data();
    io->open(QIODevice::ReadOnly|QIODevice::Unbuffered);//!!!overrides the settings //QIODevice::Unbuffered to stop reading to much. i dont know how to adjust iodevice's buffer size is seems to default to 16000bytes
}

///Disconnects the source device from the modem
void MskModulator::DisconnectSourceDevice()
{
    if(pdatasourcedevice.isNull())return;
    QIODevice *io=pdatasourcedevice.data();
    io->close();
    pdatasourcedevice.clear();
}

void MskModulator::start()
{
    bitstosendbeforereadystatesignal=settings.fb*settings.secondsbeforereadysignalemited;
    bitcounter=0;
    emit ReadyState(false);
    open(QIODevice::ReadOnly);
}

void MskModulator::stop()
{
    close();
}

qint64 MskModulator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 MskModulator::readData(char *data, qint64 maxlen)
{
    //here is the main part where things modulation gets done
    short *ptr = reinterpret_cast<short *>(data);
    for(int i=0;i<maxlen/sizeof(short);i++)
    {
        double dval=osc.WTSinValue()/2.0;

        //if time for symbol transition
        if(st.IfPassesPointNextTime())
        {

            //if we are low on bits
            if(!bc.DataAvailable)
            {

                //ask for 2 bytes (16 bits)
                if(!pdatasourcedevice.isNull())
                {
                    QIODevice *io=pdatasourcedevice.data();
                    char buf[2]={0,0};//zeros happen if source doesn't return anything or is closed
                    int readbytes=0;
                    if(io->isOpen())readbytes=io->read(buf,2);
                    if(readbytes>0)bc.LoadSymbol((uchar)buf[0]);
                    if(readbytes>1)bc.LoadSymbol((uchar)buf[1]);
                }
                 else bc.LoadSymbol(0);//this happens when no source is connected

            }

            //emits a signal when a certin number of bits have been sent. can be used for various tasks
            if(bitcounter<=bitstosendbeforereadystatesignal)
            {                
                if(bitcounter==bitstosendbeforereadystatesignal)emit ReadyState(true);
                bitcounter++;
            }

            //load bit
            bc.GetNextSymbol();
            bool bit=bc.Result;

            //set freq for bit
            if(bit)osc.SetFreq(settings.freq_center+settings.fb/4.0);
             else osc.SetFreq(settings.freq_center-settings.fb/4.0);

        }

        (*ptr)=(dval*32768.0);
        osc.WTnextFrame();
        st.WTnextFrame();
        ptr++;
    }
    return maxlen;
}
