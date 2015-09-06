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
    osc.SetFreq(settings.freq_center,settings.Fs);
    st.SetFreq(settings.fb,settings.Fs);

  /*  if(_settings.Fs!=Fs)emit SampleRateChanged(_settings.Fs);
    Fs=_settings.Fs;
    lockingbw=_settings.lockingbw;
    fb=_settings.fb;
    freq_center=_settings.freq_center;
    if(freq_center>((Fs/2.0)-(lockingbw/2.0)))freq_center=((Fs/2.0)-(lockingbw/2.0));
    symbolspercycle=_settings.symbolspercycle;
    signalthreshold=_settings.signalthreshold;

    SamplesPerSymbol=Fs/fb;
    bbnfft=pow(2,_settings.coarsefreqest_fft_power);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);
    coarsefreqestimate->setSettings(_settings.coarsefreqest_fft_power,lockingbw,fb,Fs);
    mixer_center.SetFreq(freq_center,Fs);
    mixer2.SetFreq(freq_center,Fs);

    delete matchedfilter_re;
    delete matchedfilter_im;
    matchedfilter_re = new FIR(2*SamplesPerSymbol);
    matchedfilter_im = new FIR(2*SamplesPerSymbol);
    for(int i=0;i<2*SamplesPerSymbol;i++)
    {
        matchedfilter_re->FIRSetPoint(i,sin(M_PI*i/(2.0*SamplesPerSymbol))/(2.0*SamplesPerSymbol));
        matchedfilter_im->FIRSetPoint(i,sin(M_PI*i/(2.0*SamplesPerSymbol))/(2.0*SamplesPerSymbol));
    }

    delete agc;
    agc = new AGC(4,Fs);

    delete ebnomeasure;
    ebnomeasure = new MSKEbNoMeasure(2.0*Fs);//1 second ave //SamplesPerSymbol*125);//125 symbol averaging

    sig2buff.resize(SamplesPerSymbol*symbolspercycle);
    sig2buff_ptr=0;
    sigbuff.resize(sig2buff.size()+2*SamplesPerSymbol);
    sigbuff_ptr=0;

    pointbuff.resize(100);
    pointbuff_ptr=0;
    mse=10.0;

    symbolbuff.resize(10);
    symbolbuff_ptr=0;

    lastindex=0;

    delaybuff.resize(SamplesPerSymbol);

    emit Plottables(mixer2.GetFreqHz(),mixer_center.GetFreqHz(),lockingbw);*/

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
