#include "burstmskdemodulator.h"
#include <QDebug>
#include "coarsefreqestimate.h"

#include <iostream>

#include <QTimerEvent>

BurstMskDemodulator::BurstMskDemodulator(QObject *parent)
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


    agc = new AGC(1,Fs);
    agc2 = new AGC(1,Fs);


    ebnomeasure = new MSKEbNoMeasure(0.25*Fs);//2 second ave //SamplesPerSymbol*125);//125 symbol averaging

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

    RxDataBits.reserve(1000);//unpacked
    RxDataBytes.reserve(100);//packed in bytes

    coarsefreqestimate = new CoarseFreqEstimate(this);
    coarsefreqestimate->setSettings(14,lockingbw,fb,Fs);
    connect(this, SIGNAL(BBOverlapedBuffer(const QVector<cpx_type>&)),coarsefreqestimate,SLOT(ProcessBasebandData(const QVector<cpx_type>&)));
    connect(coarsefreqestimate, SIGNAL(FreqOffsetEstimate(double)),this,SLOT(FreqOffsetEstimateSlot(double)));


    // burst stuff
    hfir=new QJHilbertFilter(this);

    fftr = new FFTr(pow(2.0,(ceil(log2(  128.0*SamplesPerSymbol   )))),false);
    mav1= new MovingAverage(SamplesPerSymbol*128);

    tridentbuffer_sz=qRound((256.0+16.0+16.0)*SamplesPerSymbol);//room for trident and preamble and a bit more
    tridentbuffer.resize(tridentbuffer_sz);
    tridentbuffer_ptr=0;

    d2.setLength(tridentbuffer_sz);//delay line for aligning demod to output of trident check and freq est


    startstop=-1;
    numPoints = 0;


}

///Connects a sink devide to the modem for the demodulated data
void BurstMskDemodulator::ConnectSinkDevice(QIODevice *_datasinkdevice)
{
    if(!_datasinkdevice)return;
    pdatasinkdevice=_datasinkdevice;
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->open(QIODevice::WriteOnly);//!!!overrides the settings
}

///Disconnects the sink devide from the modem
void BurstMskDemodulator::DisconnectSinkDevice()
{
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->close();
    pdatasinkdevice.clear();
}

void BurstMskDemodulator::setAFC(bool state)
{
    afc=state;
}

void BurstMskDemodulator::setSQL(bool state)
{
    sql=state;
}

void BurstMskDemodulator::setScatterPointType(ScatterPointType type)
{
    scatterpointtype=type;
}

double  BurstMskDemodulator::getCurrentFreq()
{
    return mixer_center.GetFreqHz();
}

void BurstMskDemodulator::invalidatesettings()
{
    Fs=-1;
    fb=-1;
}

void BurstMskDemodulator::setSettings(Settings _settings)
{
    if(_settings.Fs!=Fs)emit SampleRateChanged(_settings.Fs);
    Fs=_settings.Fs;
    lockingbw=_settings.lockingbw;
    if(_settings.fb!=fb)emit BitRateChanged(_settings.fb,true);
    fb=_settings.fb;

    //Since oqpsk
    if(fb>Fs)fb=Fs;//incase
    /*if((int(Fs/fb))!=(Fs/fb))
    {
        qDebug()<<"Note: These settings wont work for MSK";
    }*/

    freq_center=_settings.freq_center;
    if(freq_center>((Fs/2.0)-(lockingbw/2.0)))freq_center=((Fs/2.0)-(lockingbw/2.0));
    symbolspercycle=_settings.symbolspercycle;
    signalthreshold=_settings.signalthreshold;
    SamplesPerSymbol=int(Fs/fb);
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
    agc = new AGC(1,Fs);

    delete agc2;
    agc2 = new AGC(4,Fs);




    delete ebnomeasure;
    ebnomeasure = new MSKEbNoMeasure(0.25*Fs);//1 second ave //SamplesPerSymbol*125);//125 symbol averaging

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

    a1.setdelay(SamplesPerSymbol/2.0);

    symboltone_averotator=1;
    carrier_rotation_est=0;

    if(fb >= 1200){

        bt_d1.setdelay(1.0*SamplesPerSymbol);
        bt_ma1.setLength(qRound(126.0*SamplesPerSymbol));//not sure whats best

        delete mav1;
        mav1= new MovingAverage(SamplesPerSymbol*126);
        bt_ma_diff.setdelay(SamplesPerSymbol*126);//not sure whats best

        pdet.setSettings((int)(SamplesPerSymbol*126.0/2.0),0.1);

        delete fftr;
        int N=4096*4*2;
        fftr = new FFTr(N,false);


        // changed trident from 120 to 194, half of 74 + 120
        tridentbuffer_sz=qRound((200.0)*SamplesPerSymbol);//room for trident and preamble and a bit more
        tridentbuffer.resize(tridentbuffer_sz);
        tridentbuffer_ptr=0;

        //set this delay so it lines up to end of the peak detector and thus starts at the start tone of the burst
        d1.setLength(((int) 289 * SamplesPerSymbol) + 20);//adjust so start of burst aligns

        //d2.setLength(qRound((126.0)*SamplesPerSymbol) );//delay line for aligning demod to output of trident check and freq est, was 120
        d2.setLength(qRound((72.0)*SamplesPerSymbol) );//delay line for aligning demod to output of trident check and freq est, was 120

        in.resize(N);
        out_base.resize(N);
        out_top.resize(N);
        out_abs_diff.resize(N/2);

        //pdet.setSettings((int)(SamplesPerSymbol*74.0/2.0),0.2);


        startstopstart=SamplesPerSymbol*(1050);//(256+2000);//128);

        trackingDelay = 10*SamplesPerSymbol;

    }else{


        //600........
        delete mav1;
        mav1= new MovingAverage(SamplesPerSymbol*150);

        bt_ma_diff.setdelay(SamplesPerSymbol*150);//not sure whats best

        bt_d1.setdelay(1.0*SamplesPerSymbol);
        bt_ma1.setLength(qRound(150.0*SamplesPerSymbol));//not sure whats best

        pdet.setSettings((int)(SamplesPerSymbol*150.0/2.0),0.2);


        delete fftr;
        int N=4096*4*2;
        fftr = new FFTr(N,false);


        tridentbuffer_sz=qRound((224)*SamplesPerSymbol);//room for trident and preamble and a bit more
        tridentbuffer.resize(tridentbuffer_sz);
        tridentbuffer_ptr=0;

        d1.setLength(((int) 294 * SamplesPerSymbol) + 20);//adjust so start of burst aligns

        d2.setLength(qRound((150.0)*SamplesPerSymbol));//delay line for aligning demod to output of trident check and freq est

        in.resize(N);
        out_base.resize(N);
        out_top.resize(N);
        out_abs_diff.resize(N/2);

        startstopstart=SamplesPerSymbol*(1050);//(256+2000);//128);

        trackingDelay = 20*SamplesPerSymbol;

    }

    counter = 0;

    previousPhase = 0;


}

void BurstMskDemodulator::CenterFreqChangedSlot(double freq_center)//spectrum display calls this when user changes the center freq
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

BurstMskDemodulator::~BurstMskDemodulator()
{
    delete matchedfilter_re;
    delete matchedfilter_im;
    delete agc;
    delete agc2;
    delete ebnomeasure;
    delete pointmean;
}

void BurstMskDemodulator::start()
{
    open(QIODevice::WriteOnly);
}

void BurstMskDemodulator::stop()
{
    close();
}

qint64 BurstMskDemodulator::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}

qint64 BurstMskDemodulator::writeData(const char *data, qint64 len)
{

    const short *ptr = reinterpret_cast<const short *>(data);


    int numofsamples=(len/sizeof(short));

    //make analytical signal
    hfirbuff.resize(numofsamples);
    kffsamp_t asample;
    asample.i=0;

    for(int i=0;i<numofsamples;i++)
    {
        asample.r=((double)(*ptr))/32768.0;

        hfirbuff[i]=asample;

        ptr++;
    }

    hfir->Update(hfirbuff);

    //run through each sample of analyitical signal
    for(int i=0;i<hfirbuff.size();i++)
    {

        counter++;
        std::complex<double> cval=std::complex<double>(hfirbuff[i].r,hfirbuff[i].i);

        //take orginal arm
        double dval=cval.real();


        //spectrum display for looks
        if(fabs(dval)>maxval)maxval=fabs(dval);
        spectrumcycbuff[spectrumcycbuff_ptr]=dval;
        spectrumcycbuff_ptr++;spectrumcycbuff_ptr%=spectrumnfft;
        if(timer.elapsed()>150)
        {
            timer.start();
            emit OrgOverlapedBuffer(spectrumcycbuff);
            emit PeakVolume(maxval);
            maxval=0;
        }


        //agc
        agc->Update(std::abs(cval));
        cval*=agc->AGCVal;

        //delayed val for trident alignment
        std::complex<double> cval_d=d1.update_dont_touch(cval);

        //delayed val for after trident check
        double val_to_demod=d2.update_dont_touch(std::real(cval_d));

        //create burst timing signal
        double fastarm=std::abs(bt_ma1.UpdateSigned(cval*std::conj(bt_d1.update(cval))));
        fastarm=mav1->UpdateSigned(fastarm);
        fastarm-=bt_ma_diff.update(fastarm);
        if(fastarm<0)fastarm=0;
        double bt_sig=fastarm*fastarm;
        if(bt_sig>500)bt_sig=500;

        //peak detection to start filling trident buffer and flash signal LED
        if(pdet.update(bt_sig))
        {

            tridentbuffer_ptr=0;

        }

        if(tridentbuffer_ptr<tridentbuffer_sz)//fill trident buffer
        {
            tridentbuffer[tridentbuffer_ptr]=std::real(cval_d);
            tridentbuffer_ptr++;

        }
        else if(tridentbuffer_ptr==tridentbuffer_sz)//trident buffer is now filled so now check for trident and carrier freq and phase and amplitude
        {
            tridentbuffer_ptr++;

            int size_base = 126;
            int size_top = 74;

            if(fb < 1200){

                size_base = 150;
                size_top = 74;
            }

            //base
            in=tridentbuffer.mid(0,qRound(size_base*SamplesPerSymbol));
            in.resize(out_base.size());
            for(int k=qRound(size_base*SamplesPerSymbol);k<in.size();k++)in[k]=0;
            fftr->transform(in,out_base);

            //top
            in=tridentbuffer.mid(qRound(size_base*SamplesPerSymbol),qRound(size_top*SamplesPerSymbol));
            in.resize(out_top.size());

            for(int k=qRound(size_top*SamplesPerSymbol);k<in.size();k++)
            {
                in[k]=0;
            }

            fftr->transform(in,out_top);

            double hzperbin=Fs/((double)out_base.size());

            // the distance between center freq bin and real/imag arm freq
            int peakspacingbins=qRound((0.5*fb)/hzperbin);

            //find strongest base loc, this is the start tone of the preamble
            int minvalbin=0;
            double minval = 0;
            for(int i=0;i< out_base.size()/2 ;i++)
            {
                if(std::abs(out_base[i]) > minval)
                {
                    minval=std::abs(out_base[i]);
                    minvalbin=i;
                }

            }

            // find max top position. This is the 0-1 alternating part of the preable.
            double maxtop = 0;
            int maxtoppos = 0;
            for(int i=50; i < minvalbin - (peakspacingbins/2); i++){

                if(std::abs(out_top[i]) > maxtop){

                    maxtop = std::abs(out_top[i]);
                    maxtoppos = i;
                }

            }

            // ok so now we have the center bin, maxtoppos and one of the side peaks, should be to the left of the main peak, minvalbin
            int distfrompeak = std::abs(maxtoppos - minvalbin);

            // check if the side peak is within the expected range +/- 5%
            if(minval>500.0 && std::abs(distfrompeak-peakspacingbins) < std::abs(peakspacingbins/20))
            {

                //set the demodulator carrier freq and phase using estimates
                //double carrierphase=std::arg(out_base[minvalbin])-(M_PI/4.0);
                double carrierphase=std::arg(val_to_demod)-(M_PI/4.0);

                mixer2.SetFreq(hzperbin*minvalbin);
                mixer2.SetPhaseDeg((180.0/M_PI)*carrierphase);

                CenterFreqChangedSlot(hzperbin*minvalbin);

                coarsefreqestimate->bigchange();

                emit Plottables(mixer2.GetFreqHz(),mixer2.GetFreqHz(),lockingbw);
                bbcycbuff_ptr = 0;


                for(int j=0;j<bbtmpbuff.size();j++)
                {
                    bbtmpbuff[j]=0;

                }

                for(int j=0;j<delaybuff.size();j++)
                {
                    delaybuff[j]=cpx_type(0,0);

                }

                //set gain given estimate
                vol_gain=(1.4142*500.0/minval);

                pointbuff.fill(0);
                pointmean->Zero();
                startstop=startstopstart;
                cntr=0;
                emit SignalStatus(true);

                symtracker.Reset();
                sig2buff_ptr = 0;
                sigbuff_ptr = 0;

                RxDataBits.clear();
                // indicate start of burst
                RxDataBits.push_back(-1);
                RxDataBytes.clear();

//                symboltone_rotator=1;
//                symboltone_averotator=1;
                mse=0;

            }
        }//end of trident check


        //sample counting and signalstatus timout
        if(startstop>0)
        {
            startstop--;
            if(cntr<1000000)cntr++;
            if(mse<0.50)
            {
                startstop=startstopstart;
            }


        }
        if(startstop==0)
        {
            startstop--;

            emit SignalStatus(false);
            cntr = 0;
        }


        //matched filter.
        cval= mixer2.WTCISValue()*(vol_gain*val_to_demod);
        cpx_type sig2 = cpx_type(matchedfilter_re->FIRUpdateAndProcess(cval.real()),matchedfilter_im->FIRUpdateAndProcess(cval.imag()));


        //Measure ebno
        ebnomeasure->Update(std::abs(sig2));

        //AGC
        sig2*=agc2->Update(std::abs(sig2));

        //clipping
        double abval=std::abs(sig2);
        if(abval>2.84)sig2=(2.84/abval)*sig2;


        if(startstop > 0){

            //for coarse freq estimation
            bbcycbuff[bbcycbuff_ptr]=mixer_center.WTCISValue()*val_to_demod;
            bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
            if(bbcycbuff_ptr%(bbnfft/4)==0)//75% overlap
            {
                for(int j=0;j<bbcycbuff.size();j++)
                {
                    bbtmpbuff[j]=bbcycbuff[bbcycbuff_ptr];
                    bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
                }

                if(cntr==bbnfft){

                    coarsefreqestimate->bigchange();
                }
                if(cntr > bbnfft){
                    emit BBOverlapedBuffer(bbtmpbuff);
                }
            }

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

                //symbol phase tracking method 2
                if(mse>signalthreshold)symboltrackingweight=1;
                symboltrackingweight=symboltrackingweight*0.9+0.1*fabs(symerrordiff)/(M_PI_2);
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


                //Carrier tracking
                double carriererror=(pi/180.0)*(rotationest-90.0);

                symtracker.Phase=std::fmod((symtracker.Phase+pi),(2.0*pi))-pi;

                //phase adjustment for next time
                mixer2.IncresePhaseDeg((180.0/pi)*(carriererror)*0.65);

                //adjust carrier frequency for next time
                if(startstop >0 && cntr > trackingDelay ){

                    mixer2.IncreseFreqHz(1.0*0.5*0.90*(180.0/pi)*(1.0-0.9999*cos(carriererror))*(carriererror)/(360.0*symbolspercycle*SamplesPerSymbol/Fs));
                    if((mixer2.GetFreqHz()-mixer_center.GetFreqHz())>(lockingbw/2.0))
                    {
                        mixer2.SetFreq(mixer_center.GetFreqHz()+(lockingbw/2.0));
                    }
                    if((mixer2.GetFreqHz()-mixer_center.GetFreqHz())<(-lockingbw/2.0))
                    {

                        mixer2.SetFreq(mixer_center.GetFreqHz()-(lockingbw/2.0));
                    }
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
                    sigbuff[k]=sigbuff[k]*std::exp(imag*carriererror*0.5);
                }

                //create sample index and make sure we dont miss any points due to jitter
                double symboltimingstartest=SamplesPerSymbol-SamplesPerSymbol*(symtracker.Phase)/pi;
                tixd.clear();
                for(int k=0;;k++)
                {
                    int tval=round(symboltimingstartest+((double)(k-1))*2.0*SamplesPerSymbol);
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
                if(scatterpointtype==SPT_constellation){

                    if(cntr == 1480*symbolspercycle) {

                        emit ScatterPoints(pointbuff);

                    }
                }
                //send ebno when right time
                if(cntr == 1480*symbolspercycle ){

                    emit EbNoMeasurmentSignal(ebnomeasure->EbNo);

                }


                //for looks show symbol phase offset
                symbolbuff[symbolbuff_ptr]=std::exp(imag*symerrordiff);
                symbolbuff_ptr++;symbolbuff_ptr%=symbolbuff.size();
                if(scatterpointtype==SPT_phaseoffseterror)emit ScatterPoints(symbolbuff);

                singlepointphasevector[0]=std::exp(imag*symtracker.Phase);
                if(scatterpointtype==SPT_phaseoffsetest)emit ScatterPoints(singlepointphasevector);

                //soft decision, diffdecode, then push bits in a byte array

                bool thisonpt_imag = false;
                bool thisonpt_real = false;

                for(int k=0;k<tixd.size();k++)
                {
                    cpx_type thisonpt=sigbuff[tixd[k]];
                    if(thisonpt.imag()>0)thisonpt_imag=1;
                    else thisonpt_imag=0;
                    if(thisonpt.real()>0)thisonpt_real=1;
                    else thisonpt_real=0;

                    //soft BPSK x2
                    //-1 --> 0 , 1 --> 255 (-1 means 0 and 1 means 1) sort of
                    //there is no packed bits in each byte
                    //convert to the strange range for soft

                    double imag = diffdecode.UpdateSoft(thisonpt.imag());

                    int ibit=qRound((imag)*127.0+128.0);
                    if(ibit>255)ibit=255;
                    if(ibit<0)ibit=0;

                    RxDataBits.push_back((uchar)ibit);

                    double real = diffdecode.UpdateSoft(thisonpt.real());
                    real =- real;

                    ibit=qRound((real)*127.0+128.0);

                    if(ibit>255)ibit=255;
                    if(ibit<0)ibit=0;

                    RxDataBits.push_back((uchar)ibit);

                }


                //return symbol phase
                emit SymbolPhase(symtracker.Phase);

                //calc MSE of the points

                mse=0;
                // check so we average correctly when points less than pointbuf size
                int pointsloaded = cntr/SamplesPerSymbol/2;
                if(pointsloaded > pointbuff.size()) pointsloaded=pointbuff.size();
                double average = pointmean->Val*100/pointsloaded;


                for(int k=0;k<pointsloaded;k++)
                {
                    double tda,tdb;
                    cpx_type tcpx;
                    tcpx=sqrt(2)*pointbuff[k]/average;
                    tda=(fabs(tcpx.real())-1.0);
                    tdb=(fabs(tcpx.imag())-1.0);
                    mse+=(tda*tda)+(tdb*tdb);
                }
                mse/=((double)pointsloaded);

                /*
                if(cntr < 30000){
                   std::cout << "symboltimginphaseest " << symboltimginphaseest << "rotation estimate " << rotationest << " symboltimingstartest " << symboltimingstartest << " tracker phase " << symtracker.Phase << " freq " << symtracker.Freq << " carrier error " << carriererror << " " << mse << "\r\n";

                }
                */
                if(!RxDataBits.isEmpty()){

                    emit processDemodulatedSoftBits(RxDataBits);
                    RxDataBits.clear();
                }

            }

            mixer2.WTnextFrame();
            mixer_center.WTnextFrame();
            ptr++;

        }
    }



    return len;

}

void BurstMskDemodulator::FreqOffsetEstimateSlot(double freq_offset_est)//coarse est class calls this with current est
{

    static int countdown=0;
    if((mse>signalthreshold)&&(fabs(mixer2.GetFreqHz()-(mixer_center.GetFreqHz()+freq_offset_est))>1.0))//no sig, prob cant track carrier phase
    {


        mixer2.SetFreq(mixer_center.GetFreqHz()+freq_offset_est);

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

    emit MSESignal(mse);

    if(mse>signalthreshold)emit SignalStatus(false);
    else emit SignalStatus(true);

}




