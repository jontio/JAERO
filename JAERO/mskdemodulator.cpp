#include "mskdemodulator.h"
#include <QDebug>
#include "coarsefreqestimate.h"

#include <iostream>

#include <QTimerEvent>

MskDemodulator::MskDemodulator(QObject *parent)
    :   QIODevice(parent)
{

    afc=false;
    sql=false;

    timer.start();

    Fs=48000;
    double freq_center=1000;
    lockingbw=900;
    fb=600;

    signalthreshold=0.5;//lower is less sensitive

    SamplesPerSymbol=Fs/fb;

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

    agc = new AGC(1,Fs);

    ebnomeasure = new MSKEbNoMeasure(2.0*Fs);//2 second ave //SamplesPerSymbol*125);//125 symbol averaging

    pointmean = new MovingAverage(100);

    mixer_center.SetFreq(freq_center,Fs);
    mixer2.SetFreq(freq_center,Fs);

    st_osc.SetFreq(fb/2,Fs);

    bbnfft=pow(2,14);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);

    spectrumnfft=pow(2,13);
    spectrumcycbuff.resize(spectrumnfft);
    spectrumcycbuff_ptr=0;
    spectrumtmpbuff.resize(spectrumnfft);

    pointbuff.resize(100);
    pointbuff_ptr=0;
    mse=10.0;
    msema = new MovingAverage(600);

    lastindex=0;

    singlepointphasevector.resize(1);

    marg= new MovingAverage(80);
    dt.setLength(40);

    RxDataBits.reserve(100);//unpacked

    coarsefreqestimate = new CoarseFreqEstimate(this);
    coarsefreqestimate->setSettings(14,lockingbw,fb,Fs);
    connect(this, SIGNAL(BBOverlapedBuffer(const QVector<cpx_type>&)),coarsefreqestimate,SLOT(ProcessBasebandData(const QVector<cpx_type>&)));
    connect(coarsefreqestimate, SIGNAL(FreqOffsetEstimate(double)),this,SLOT(FreqOffsetEstimateSlot(double)));

    dcd = false;

    correctionfactor=1.0;

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

void MskDemodulator::invalidatesettings()
{
    Fs=-1;
    fb=-1;
}

void MskDemodulator::setSettings(Settings _settings)
{
    if(_settings.Fs!=Fs)emit SampleRateChanged(_settings.Fs);
    Fs=_settings.Fs;
    lockingbw=_settings.lockingbw;
    if(_settings.fb!=fb)emit BitRateChanged(_settings.fb,false);
    fb=_settings.fb;

    freq_center=_settings.freq_center;
    if(freq_center>((Fs/2.0)-(lockingbw/2.0)))freq_center=((Fs/2.0)-(lockingbw/2.0));

    signalthreshold=_settings.signalthreshold;

    SamplesPerSymbol=int(Fs/fb);
    bbnfft=pow(2,_settings.coarsefreqest_fft_power);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);
    coarsefreqestimate->setSettings(_settings.coarsefreqest_fft_power,lockingbw,fb,Fs);

    mixer_center.SetFreq(freq_center,Fs);
    mixer2.SetFreq(freq_center,Fs);

    st_osc.SetFreq(fb/2,Fs);

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
    agc = new AGC(1,Fs);

    delete ebnomeasure;
    ebnomeasure = new MSKEbNoMeasure(2.0*Fs);//1 second ave //SamplesPerSymbol*125);//125 symbol averaging

    pointbuff.resize(100);
    pointbuff_ptr=0;
    mse=10.0;

    lastindex=0;

    emit Plottables(mixer2.GetFreqHz(),mixer_center.GetFreqHz(),lockingbw);

    st_iir_resonator.a.resize(3);
    st_iir_resonator.b.resize(3);

    if(fb >= 1200)
    {
        correctionfactor = 0.6;

        if(Fs==48000)
        {

            // 600hz / 4hz / 48000
            st_iir_resonator.a[0]=1;
            st_iir_resonator.a[1]= -1.993312819378528;
            st_iir_resonator.a[2]= 0.999476538254407;
            st_iir_resonator.b[0]=2.617308727964618e-04;
            st_iir_resonator.b[1]=0;
            st_iir_resonator.b[2]=-2.617308727964618e-04;

            ee=0.025;
        }
        else
        {
            // 600hz / 4hz / 24000
            st_iir_resonator.a[0]=1;
            st_iir_resonator.a[1]=-1.974342917561558;
            st_iir_resonator.a[2]=0.998953350377616;
            st_iir_resonator.b[0]=5.233248111921052e-04;
            st_iir_resonator.b[1]= 0;
            st_iir_resonator.b[2]=-5.233248111921052e-04;

            ee=0.05;
        }

    }
    else
    {

        correctionfactor = 1.0;

        if(Fs==48000)
        {
            // 300hz / 4hz / 48000
            st_iir_resonator.a[0]=1;
            st_iir_resonator.a[1]=-1.998196509168551;
            st_iir_resonator.a[2]=0.999738234875681;
            st_iir_resonator.b[0]=1.308825621597620e-04;
            st_iir_resonator.b[1]=0;
            st_iir_resonator.b[2]=-1.308825621597620e-04;

            ee=0.025;

        }else
        {
            // 300hz / 4hz / 12000
            st_iir_resonator.a[0]=1;
            st_iir_resonator.a[1]=-1.974342917561558;
            st_iir_resonator.a[2]=0.998953350377616;
            st_iir_resonator.b[0]=5.233248111921052e-04;
            st_iir_resonator.b[1]=0;
            st_iir_resonator.b[2]=-5.233248111921052e-04;

            ee=0.0125;

        }
    }

    st_iir_resonator.init();

    delete marg;
    marg= new MovingAverage(SamplesPerSymbol);
    dt.setLength(SamplesPerSymbol/2);

    delayedsmpl.setLength(SamplesPerSymbol);

    delayt8.setdelay((SamplesPerSymbol)/2.0);

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
    delete msema;
    delete ebnomeasure;
    delete pointmean;
    delete marg;

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

    bool sendscatterpoints=false;

    const short *ptr = reinterpret_cast<const short *>(data);
    for(int i=0;i<len/sizeof(short);i++)
    {

        double dval=((double)(*ptr))/32768.0;

        //for looks
        spectrumcycbuff[spectrumcycbuff_ptr]=dval;
        spectrumcycbuff_ptr++;spectrumcycbuff_ptr%=spectrumnfft;
        if(spectrumcycbuff_ptr%(spectrumnfft/4)==0)
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
                sendscatterpoints=true;

            }
        }

        //for coarse freq estimation
        bbcycbuff[bbcycbuff_ptr]=mixer_center.WTCISValue()*dval;
        bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
        if(bbcycbuff_ptr%(bbnfft)==0)//75% overlap
        {
            for(int j=0;j<bbcycbuff.size();j++)
            {
                bbtmpbuff[j]=bbcycbuff[bbcycbuff_ptr];
                bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
            }
            emit BBOverlapedBuffer(bbtmpbuff);
        }


        cpx_type cval= mixer2.WTCISValue()*(dval);
        cpx_type sig2 = cpx_type(matchedfilter_re->FIRUpdateAndProcess(cval.real()),matchedfilter_im->FIRUpdateAndProcess(cval.imag()));

        //Measure ebno
        ebnomeasure->Update(std::abs(sig2));

        //AGC
        sig2*=agc->Update(std::abs(sig2));

        //clipping
        double abval=std::abs(sig2);
        if(abval>2.84)sig2=(2.84/abval)*sig2;

        cpx_type pt_d = delayedsmpl.update_dont_touch(sig2);
        cpx_type pt_msk=cpx_type(sig2.real(), pt_d.imag());

        double st_eta = st_iir_resonator.update(std::abs(pt_msk));

        cpx_type st_m1=cpx_type(st_eta,-delayt8.update(st_eta));
        cpx_type st_out=st_osc.WTCISValue()*st_m1;

        double st_angle_error=std::arg(st_out);

        //adjust frequency and phase of symbol timing
        double weighting=fabs(tanh(st_angle_error));

        if(!dcd)
        {
            st_osc.AdvanceFractionOfWave(-(1.0-weighting)*st_angle_error*(0.05/360.0));

        }
        else
        {
            st_osc.AdvanceFractionOfWave(-(1.0-weighting)*st_angle_error*(0.003/360.0));
        }

        //sample times
        if(st_osc.IfHavePassedPoint(ee))
        {

            //carrier tracking
            double ct_xt=tanh(sig2.imag())*sig2.real();
            double ct_xt_d=tanh(pt_d.real())*pt_d.imag();
            double ct_ec=ct_xt_d-ct_xt;


            if(ct_ec>M_PI)ct_ec=M_PI;
            if(ct_ec<-M_PI)ct_ec=-M_PI;
            if(ct_ec>M_PI_2)ct_ec=M_PI_2;
            if(ct_ec<-M_PI_2)ct_ec=-M_PI_2;

            double carrier_aggression=12.0*correctionfactor;
            if(dcd)carrier_aggression=8.0*correctionfactor;

            mixer2.IncresePhaseDeg(carrier_aggression*1.0*ct_ec);
            mixer2.IncreseFreqHz(carrier_aggression*0.01*ct_ec);

            //rotate to remove any remaining bias
            marg->UpdateSigned(ct_ec/2.0);
            dt.update(pt_msk);
            pt_msk*=cpx_type(cos(marg->Val),sin(marg->Val));

            //gui feedback
            static int slowdown=0;
            if(pointbuff_ptr==0){slowdown++;slowdown%=12;}
            //for looks show constellation
            if(!slowdown)
            {
                ASSERTCH(pointbuff,pointbuff_ptr);
                pointbuff[pointbuff_ptr]=pt_msk*0.75;

                pointbuff_ptr++;pointbuff_ptr%=pointbuff.size();
                if(scatterpointtype==SPT_constellation&&sendscatterpoints){sendscatterpoints=false;emit ScatterPoints(pointbuff);}
            }

            double tda=(fabs((pt_msk).real()*0.75)-1.0);
            double tdb=(fabs((pt_msk).imag()*0.75)-1.0);
            mse=msema->Update((tda*tda)+(tdb*tdb));

            //soft bits
            double imagin = diffdecode.UpdateSoft(pt_msk.imag());

            int ibit=qRound((imagin)*127.0+128.0);
            if(ibit>255)ibit=255;
            if(ibit<0)ibit=0;

            RxDataBits.push_back((uchar)ibit);

            double real = diffdecode.UpdateSoft(pt_msk.real());

            real =- real;

            ibit=qRound((real)*127.0+128.0);

            if(ibit>255)ibit=255;
            if(ibit<0)ibit=0;


            RxDataBits.push_back((uchar)ibit);

            // push them out to decode
            if(RxDataBits.size() >= 12)
            {
                emit processDemodulatedSoftBits(RxDataBits);
                RxDataBits.clear();

            }
        }

        mixer2.WTnextFrame();
        mixer_center.WTnextFrame();

        st_osc.WTnextFrame();
        ptr++;
    }
    return len;

}

void MskDemodulator::FreqOffsetEstimateSlot(double freq_offset_est)//coarse est class calls this with current est
{

    static int countdown=4;
    if((mse>signalthreshold)&&(fabs(mixer2.GetFreqHz()-(mixer_center.GetFreqHz()+freq_offset_est))>0.0))//no sig, prob cant track carrier phase
    {
        mixer2.SetFreq(mixer_center.GetFreqHz()+freq_offset_est);
    }
    if((afc)&&(dcd)&&(fabs(mixer2.GetFreqHz()-mixer_center.GetFreqHz())>2.0))//got a sig, afc on, freq is getting a little to far out
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

void MskDemodulator::DCDstatSlot(bool _dcd)
{

    dcd=_dcd;

}




