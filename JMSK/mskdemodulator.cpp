#include "mskdemodulator.h"
#include <QDebug>
#include "coarsefreqestimate.h"

#include <assert.h>
#include <iostream>

#include <QTimerEvent>

MskDemodulator::MskDemodulator(QObject *parent)
:   QIODevice(parent)
{

    afc=false;
    sql=false;

    timer.start();

    Fs=8000;
    double freq_center=1000;
    lockingbw=500;
    fb=125;
    symbolspercycle=8;//about 8 for 50bps, 16 or 24 ish for 125 and up seems ok
    signalthreshold=0.5;//lower is less sensitive

    SamplesPerSymbol=Fs/fb;

    symboltrackingweight=1.0;

    scatterpointtype=SPT_constellation;

    bc.SetInNumberOfBits(1);
    bc.SetOutNumberOfBits(8);

    matchedfilter_re = new FIR(2*SamplesPerSymbol);
    matchedfilter_im = new FIR(2*SamplesPerSymbol);
    for(int i=0;i<2*SamplesPerSymbol;i++)
    {
        matchedfilter_re->FIRSetPoint(i,sin(M_PI*i/(2.0*SamplesPerSymbol))/(2.0*SamplesPerSymbol));
        matchedfilter_im->FIRSetPoint(i,sin(M_PI*i/(2.0*SamplesPerSymbol))/(2.0*SamplesPerSymbol));
    }

    agc = new AGC(4,Fs);

    ebnomeasure = new MSKEbNoMeasure(2.0*Fs);//2 second ave //SamplesPerSymbol*125);//125 symbol averaging

    pointmean = new MovingAverage(100);

    mixer_center.SetFreq(freq_center,Fs);
    mixer2.SetFreq(freq_center,Fs);

    bbnfft=pow(2,14);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);

    spectrumnfft=pow(2,13);
    spectrumcycbuff.resize(spectrumnfft);
    spectrumcycbuff_ptr=0;
    spectrumtmpbuff.resize(spectrumnfft);

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

    singlepointphasevector.resize(1);

    //RxDataBits.reserve(1000);//unpacked
    RxDataBytes.reserve(100);//packed in bytes

    coarsefreqestimate = new CoarseFreqEstimate(this);
    coarsefreqestimate->setSettings(14,lockingbw,fb,Fs);
    connect(this, SIGNAL(BBOverlapedBuffer(const QVector<cpx_type>&)),coarsefreqestimate,SLOT(ProcessBasebandData(const QVector<cpx_type>&)));
    connect(coarsefreqestimate, SIGNAL(FreqOffsetEstimate(double)),this,SLOT(FreqOffsetEstimateSlot(double)));

}

///Connects a sink devide to the modem for the demodulated data
void MskDemodulator::ConnectSinkDevice(QIODevice *_datasinkdevice)
{
    if(!_datasinkdevice)return;
    pdatasinkdevice=_datasinkdevice;
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->open(QIODevice::WriteOnly);//!!!overrides the settings
}

///Disconnects the sink devide from the modem
void MskDemodulator::DisconnectSinkDevice()
{
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->close();
    pdatasinkdevice.clear();
}

void MskDemodulator::setAFC(bool state)
{
    afc=state;
}

void MskDemodulator::setSQL(bool state)
{
    sql=state;
}

void MskDemodulator::setScatterPointType(ScatterPointType type)
{
    scatterpointtype=type;
}

double  MskDemodulator::getCurrentFreq()
{
    return mixer_center.GetFreqHz();
}

void MskDemodulator::setSettings(Settings _settings)
{
    if(_settings.Fs!=Fs)emit SampleRateChanged(_settings.Fs);
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

    emit Plottables(mixer2.GetFreqHz(),mixer_center.GetFreqHz(),lockingbw);

}

void MskDemodulator::CenterFreqChangedSlot(double freq_center)//spectrum display calls this when user changes the center freq
{
    if(freq_center<(0.75*fb))freq_center=0.75*fb;
    if(freq_center>(Fs/2.0-0.75*fb))freq_center=Fs/2.0-0.75*fb;
    mixer_center.SetFreq(freq_center,Fs);
    if(afc)mixer2.SetFreq(mixer_center.GetFreqHz());
    if((mixer2.GetFreqHz()-mixer_center.GetFreqHz())>(lockingbw/2.0))
    {
        mixer2.SetFreq(mixer_center.GetFreqHz()+(lockingbw/2.0));
    }
    if((mixer2.GetFreqHz()-mixer_center.GetFreqHz())<(-lockingbw/2.0))
    {
        mixer2.SetFreq(mixer_center.GetFreqHz()-(lockingbw/2.0));
    }
    for(int j=0;j<bbcycbuff.size();j++)bbcycbuff[j]=0;
    emit Plottables(mixer2.GetFreqHz(),mixer_center.GetFreqHz(),lockingbw);
}

MskDemodulator::~MskDemodulator()
{
    delete matchedfilter_re;
    delete matchedfilter_im;
    delete agc;
    delete ebnomeasure;
    delete pointmean;
}

void MskDemodulator::start()
{
    open(QIODevice::WriteOnly);
}

void MskDemodulator::stop()
{
    close();
}

qint64 MskDemodulator::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}

qint64 MskDemodulator::writeData(const char *data, qint64 len)
{

    const short *ptr = reinterpret_cast<const short *>(data);
    for(int i=0;i<len/sizeof(short);i++)
    {
        double dval=((double)(*ptr))/32768.0;

        //for looks
        spectrumcycbuff[spectrumcycbuff_ptr]=dval;
        spectrumcycbuff_ptr++;spectrumcycbuff_ptr%=spectrumnfft;
        if(spectrumcycbuff_ptr%(spectrumnfft/4)==0)//75% overlap
        {
            double maxval=0;
            for(int j=0;j<spectrumcycbuff.size();j++)
            {
                spectrumtmpbuff[j]=spectrumcycbuff[spectrumcycbuff_ptr];
                spectrumcycbuff_ptr++;spectrumcycbuff_ptr%=spectrumnfft;
                if(fabs(spectrumtmpbuff[j])>maxval)
                {
                    maxval=fabs(spectrumtmpbuff[j]);
                }
            }

            if(timer.elapsed()>100)
            {
                timer.start();
                emit OrgOverlapedBuffer(spectrumtmpbuff);
                emit PeakVolume(maxval);
            }
        }

        //for coarse freq estimation
        bbcycbuff[bbcycbuff_ptr]=mixer_center.WTCISValue()*dval;
        bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
        if(bbcycbuff_ptr%(bbnfft/4)==0)//75% overlap
        {
            for(int j=0;j<bbcycbuff.size();j++)
            {
                bbtmpbuff[j]=bbcycbuff[bbcycbuff_ptr];
                bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
            }
            emit BBOverlapedBuffer(bbtmpbuff);
        }



        //matched filter.
        cpx_type cval= mixer2.WTCISValue()*dval;
        cpx_type sig2 = cpx_type(matchedfilter_re->FIRUpdateAndProcess(cval.real()),matchedfilter_im->FIRUpdateAndProcess(cval.imag()));

        //Measure ebno
        ebnomeasure->Update(std::abs(sig2));

        //AGC
        sig2*=agc->Update(std::abs(sig2));

        //clipping
        double abval=std::abs(sig2);
        if(abval>2.84)sig2=(2.84/abval)*sig2;

        //buffering
        sig2buff[sig2buff_ptr]=sig2;
        sig2buff_ptr++;sig2buff_ptr%=sig2buff.size();

        if(!sig2buff_ptr)
        {

            bool symbolfreqtofast=false;
            bool symbolfreqtoslow=false;

            double expectedhalfsymbolratepeakloc=symbolspercycle/2.0;
            const double pi = std::acos(-1);
            const cpx_type imag(0, 1);

            cpx_type testsig;
            cpx_type val_dft;
            double aval_dft;
            double N=sig2buff.size();

            double symboltimginphaseest;
            double rotationest;
            double aval_dft_max;
            symboltimginphaseest=0;
            rotationest=0;
            aval_dft_max=-1;
            for(double rkl=0;rkl<90;rkl+=1.0)//select rotation
            {

                if((rkl>15)&&(rkl<75))rkl+=(5.0-1.0);//lower density when father away from the nominal place to reduce cpu load

                //rotate, square, take real, do 1bin DFT
                val_dft=0;
                for(int j=0;j<N;j++)
                {
                    testsig=(sig2buff[sig2buff_ptr]*std::exp(imag*((double)rkl)*pi/180.0));//rotate
                    testsig*=testsig;//square
                    testsig=testsig.real();//take real
                    testsig*=std::exp(-imag*2.0*pi*((double)j)*(expectedhalfsymbolratepeakloc)/N);//part 1 of DTF
                    val_dft+=testsig;//part 2 of DFT
                    sig2buff_ptr++;sig2buff_ptr%=sig2buff.size();
                }

                //choose the max abs(val_dft)
                aval_dft=std::abs(val_dft);
                if(aval_dft>aval_dft_max)
                {
                    aval_dft_max=aval_dft;
                    rotationest=rkl;
                    symboltimginphaseest=std::arg(val_dft);
                }

            }

            //T/2 symbol tracking on real oscilations and reduction of carrier ambiguity
            double symerrordiff1=std::arg(std::exp(imag*symboltimginphaseest)*std::exp(-imag*symtracker.Phase));
            double symerrordiff2=std::arg(std::exp(imag*(symboltimginphaseest+pi))*std::exp(-imag*symtracker.Phase));
            double symerrordiff=symerrordiff1;
            if(fabs(symerrordiff2)<fabs(symerrordiff1))
            {
                symerrordiff=symerrordiff2;
                rotationest=rotationest+90.0;//reduction of carrier ambiguity
            }
            symtracker.Phase+=symtracker.Freq;
            //symbol phase tracking method 1
            //symtracker.Phase+=(symerrordiff*0.33);//0.233
            //symtracker.Freq+=(0.001*symerrordiff);//0.0005
            //
            //symbol phase tracking method 2
            if(mse>signalthreshold)symboltrackingweight=1;
             else symboltrackingweight=symboltrackingweight*0.9+0.1*fabs(symerrordiff)/(M_PI_2);
            if(symboltrackingweight<0.001)symboltrackingweight=0.001;
            if(symboltrackingweight>1.0)symboltrackingweight=1.0;
            symtracker.Phase+=(symerrordiff*0.333*std::max(symboltrackingweight,0.5));
            symtracker.Freq+=(0.01*symerrordiff*symboltrackingweight);
            if(symerrordiff>0.0)symtracker.Freq+=0.001*symboltrackingweight;
             else symtracker.Freq-=0.001*symboltrackingweight;
            //
            if (symtracker.Freq>0.3)
            {
                symtracker.Freq=0.3;
                symbolfreqtofast=true;
            }
             else if (symtracker.Freq<-0.3)
             {
                symtracker.Freq=-0.3;
                symbolfreqtoslow=true;
             }
            symtracker.Phase=std::fmod((symtracker.Phase+pi),(2.0*pi))-pi;

            //Carrier tracking
            double carriererror=(pi/180.0)*(rotationest-90.0);
            //phase adjustment for next time
            mixer2.IncresePhaseDeg((180.0/pi)*(carriererror)*0.65);
            //adjust carrier frequency for next time
            mixer2.IncreseFreqHz(1.0*0.5*0.90*(180.0/pi)*(1.0-0.9999*cos(carriererror))*(carriererror)/(360.0*symbolspercycle*SamplesPerSymbol/Fs));
            if((mixer2.GetFreqHz()-mixer_center.GetFreqHz())>(lockingbw/2.0))
            {
                mixer2.SetFreq(mixer_center.GetFreqHz()+(lockingbw/2.0));
            }
            if((mixer2.GetFreqHz()-mixer_center.GetFreqHz())<(-lockingbw/2.0))
            {
                mixer2.SetFreq(mixer_center.GetFreqHz()-(lockingbw/2.0));
            }


            //delay imag arm and keep 2 symbols from last time
            for(int k=0;k<delaybuff.size();k++)
            {
                sigbuff[sigbuff_ptr]=cpx_type(sig2buff[k].real(),delaybuff[k].imag());
                sigbuff_ptr++;sigbuff_ptr%=sigbuff.size();
                delaybuff[k]=sig2buff[sig2buff.size()-delaybuff.size()+k];
            }
            for(int k=delaybuff.size();k<sig2buff.size();k++)
            {
                sigbuff[sigbuff_ptr]=cpx_type(sig2buff[k].real(),sig2buff[k-delaybuff.size()].imag());
                sigbuff_ptr++;sigbuff_ptr%=sigbuff.size();
            }

            //correct any residule rotaion (optional not sure if better or worse atm)
            for(int k=0;k<sigbuff.size();k++)
            {
          //      sigbuff[k]=sigbuff[k]*std::exp(imag*carriererror*0.5);
            }

            //create sample index and make sure we dont miss any points due to jitter
            double symboltimingstartest=SamplesPerSymbol-SamplesPerSymbol*(symtracker.Phase)/pi;
            tixd.clear();
            for(int k=0;;k++)
            {
                int tval=round(symboltimingstartest+((double)k)*2.0*SamplesPerSymbol);
                if(tval>=sigbuff.size())break;
                if((tval>=0)&&(tval<sigbuff.size())&&((tval-lastindex)>=SamplesPerSymbol))
                {
                    lastindex=tval-sig2buff.size();
                    tixd.push_back((sigbuff_ptr+tval)%sigbuff.size());
                }
            }

            //for looks show the points and keep track of the average magnitude of the points for mse calculation
            for(int k=0;k<tixd.size();k++)
            {
                pointbuff[pointbuff_ptr]=sigbuff[tixd[k]]*0.75;
                pointmean->Update(std::abs(pointbuff[pointbuff_ptr]));
                pointbuff_ptr++;pointbuff_ptr%=pointbuff.size();
            }
            if(scatterpointtype==SPT_constellation)emit ScatterPoints(pointbuff);

            //for looks show symbol phase offset
            symbolbuff[symbolbuff_ptr]=std::exp(imag*symerrordiff);
            symbolbuff_ptr++;symbolbuff_ptr%=symbolbuff.size();
            if(scatterpointtype==SPT_phaseoffseterror)emit ScatterPoints(symbolbuff);

            singlepointphasevector[0]=std::exp(imag*symtracker.Phase);
            if(scatterpointtype==SPT_phaseoffsetest)emit ScatterPoints(singlepointphasevector);

            //hard decision, diffdecode, then push bits in a byte array
            bool thisonpt_imag;
            bool thisonpt_real;
            for(int k=0;k<tixd.size();k++)
            {
                cpx_type thisonpt=sigbuff[tixd[k]];
                if(thisonpt.imag()>0)thisonpt_imag=1;
                 else thisonpt_imag=0;
                if(thisonpt.real()>0)thisonpt_real=1;
                 else thisonpt_real=0;

                //if you want packed bits
                bc.LoadSymbol(diffdecode.Update(thisonpt_imag));
                bc.LoadSymbol(1-diffdecode.Update(thisonpt_real));
                while(bc.DataAvailable)
                {
                    bc.GetNextSymbol();
                    RxDataBytes.push_back((uchar)bc.Result);
                }

                //if you want unpacked bits
                //RxDataBits.push_back(diffdecode.Update(thisonpt_imag));
                //RxDataBits.push_back(1-diffdecode.Update(thisonpt_real));

            }

            //return symbol phase
            emit SymbolPhase(symtracker.Phase);

            //calc MSE of the points
            double lastmse=mse;
            mse=0;
            for(int k=0;k<pointbuff.size();k++)
            {
                double tda,tdb;
                cpx_type tcpx;
                tcpx=sqrt(2)*pointbuff[k]/pointmean->Val;
                tda=(fabs(tcpx.real())-1.0);
                tdb=(fabs(tcpx.imag())-1.0);
                mse+=(tda*tda)+(tdb*tdb);
            }
            mse/=((double)pointbuff.size());

            //return the demodulated bits (unpacked bits)
            //if(!RxDataBits.isEmpty())
            //{
            //    if(!sql||mse<signalthreshold||lastmse<signalthreshold)emit RxData(RxDataBits);
            //    RxDataBits.clear();
            //}

            //return the demodulated data (packed in bytes)
            //using bytes and the qiodevice class
            if(!RxDataBytes.isEmpty())
            {
                if(!sql||mse<signalthreshold||lastmse<signalthreshold)
                {
                    if(!pdatasinkdevice.isNull())
                    {
                        QIODevice *io=pdatasinkdevice.data();
                        if(io->isOpen())io->write(RxDataBytes);
                    }
                }
                RxDataBytes.clear();
            }

            if(symbolfreqtofast)emit WarningTextSignal("Symbol rate to fast");
            if(symbolfreqtoslow)emit WarningTextSignal("Symbol rate to slow");

        }

        mixer2.WTnextFrame();
        mixer_center.WTnextFrame();
        ptr++;
    }

    return len;

}

void MskDemodulator::FreqOffsetEstimateSlot(double freq_offset_est)//coarse est class calls this with current est
{
    static int countdown=4;
    if((mse>signalthreshold)&&(fabs(mixer2.GetFreqHz()-(mixer_center.GetFreqHz()+freq_offset_est))>1.0))//no sig, prob cant track carrier phase
    {
        mixer2.SetFreq(mixer_center.GetFreqHz()+freq_offset_est);
        symtracker.Reset();
    }
    if((afc)&&(mse<signalthreshold)&&(fabs(mixer2.GetFreqHz()-mixer_center.GetFreqHz())>1.0))//got a sig, afc on, freq is getting a little to far out
    {
        if(countdown>0)countdown--;
         else
         {
            mixer_center.SetFreq(mixer2.GetFreqHz());
            if(mixer_center.GetFreqHz()<lockingbw/2.0)mixer_center.SetFreq(lockingbw/2.0);
            if(mixer_center.GetFreqHz()>(Fs/2.0-lockingbw/2.0))mixer_center.SetFreq(Fs/2.0-lockingbw/2.0);
            coarsefreqestimate->bigchange();
            for(int j=0;j<bbcycbuff.size();j++)bbcycbuff[j]=0;
         }
    } else countdown=4;
    emit Plottables(mixer2.GetFreqHz(),mixer_center.GetFreqHz(),lockingbw);

    emit EbNoMeasurmentSignal(ebnomeasure->EbNo);

    emit MSESignal(mse);

    if(mse>signalthreshold)emit SignalStatus(false);
     else emit SignalStatus(true);

}




