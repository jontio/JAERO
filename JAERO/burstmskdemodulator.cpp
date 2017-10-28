#include "burstmskdemodulator.h"
#include <QDebug>
#include "coarsefreqestimate.h"

#include <iostream>

#include <QTimerEvent>
#include <complex.h>
#include <math.h>

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

    st_osc.SetFreq(fb,Fs);
    st_osc_ref.SetFreq(fb,Fs);
    st_osc_quarter.SetFreq(fb/4.0,Fs);

    pt_d=0;
    sig2_last=0;
    msema = new MovingAverage(128);

    //st 10500hz resonator at 48000fps
    st_iir_resonator.a.resize(3);
    st_iir_resonator.b.resize(3);

    //[3.271421893958904e-04 0 -3.271421893958904e-04]  [1 -1.974730452137909 0.999345715621208],

    st_iir_resonator.b[0]=3.271421893958904e-04;
    st_iir_resonator.b[1]=0;
    st_iir_resonator.b[2]=-3.271421893958904e-04;
    st_iir_resonator.a[0]=1;
    st_iir_resonator.a[1]=-1.974730452137909;
    st_iir_resonator.a[2]=0.999345715621208;
    st_iir_resonator.init();

    //st delays
    delays.setdelay(1);
    delayt41.setdelay((SamplesPerSymbol)/4.0);
    delayt42.setdelay((SamplesPerSymbol)/4.0);
    delayt8.setdelay((SamplesPerSymbol)/4.0);//??


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

    if(fb>Fs)fb=Fs;//incase

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
    agc2 = new AGC(SamplesPerSymbol*32.0/Fs,Fs);

    delete ebnomeasure;
    ebnomeasure = new MSKEbNoMeasure(0.25*Fs);//1 second ave //SamplesPerSymbol*125);//125 symbol averaging

    pointbuff.resize(100);
    pointbuff_ptr=0;
    mse=10.0;

    emit Plottables(mixer2.GetFreqHz(),mixer_center.GetFreqHz(),lockingbw);

    a1.setdelay(SamplesPerSymbol);
    ee=0.05;
    symboltone_averotator=1;
    rotator=1;


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
        d2.setLength(qRound(72+120.0)*SamplesPerSymbol);//delay line for aligning demod to output of trident check and freq est, was 120

        in.resize(N);
        out_base.resize(N);
        out_top.resize(N);
        out_abs_diff.resize(N/2);

        startstopstart=SamplesPerSymbol*(2100);
        trackingDelay = 192*SamplesPerSymbol;

        endRotation = (120+56)*SamplesPerSymbol;


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

        trackingDelay = (10*SamplesPerSymbol) + 120*SamplesPerSymbol;

    }


    st_osc.SetFreq(fb,Fs);
    st_osc_ref.SetFreq(fb,Fs);
    st_osc_quarter.SetFreq(fb/4.0,Fs);
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
        if(pdet.update(bt_sig) && !(mse < signalthreshold))
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



            debug="";

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
            int maxtopposhigh =0;
            double maxtophigh =0;

            for(int i=0; i < out_top.size(); i++){

                if(i > 50){
                    if((i < minvalbin - (peakspacingbins/2)) &&  std::abs(out_top[i]) > maxtop){

                        maxtop = std::abs(out_top[i]);
                        maxtoppos = i;
                    }

                    if((i > minvalbin + (peakspacingbins/2)) &&  std::abs(out_top[i]) > maxtophigh){

                        maxtophigh = std::abs(out_top[i]);
                        maxtopposhigh = i;
                    }
                }

            }

            // ok so now we have the center bin, maxtoppos and one of the side peaks, should be to the left of the main peak, minvalbin
            int distfrompeak = std::abs(maxtoppos - minvalbin);

            // check if the side peak is within the expected range +/- 5%
            if(minval>500.0 && std::abs(distfrompeak-peakspacingbins) < std::abs(peakspacingbins/20) && !(mse<signalthreshold))

            {

                //set gain given estimate
                vol_gain=1.4142*(500.0/(minval));

                double carrierphase=std::arg(out_base[minvalbin])-(M_PI/4.0);
                mixer2.SetPhaseDeg((180.0/M_PI)*carrierphase);

                CenterFreqChangedSlot(((maxtopposhigh+maxtoppos)/2)*hzperbin);

                coarsefreqestimate->bigchange();

                emit Plottables(mixer2.GetFreqHz(),mixer2.GetFreqHz(),lockingbw);


                pointbuff.fill(0);
                pointmean->Zero();
                pointbuff_ptr=0;
                startstop=startstopstart;
                cntr=0;
                emit SignalStatus(true);

                symtracker.Reset();
                sig2buff_ptr = 0;
                sigbuff_ptr = 0;

                RxDataBits.clear();
                // indicate start of burst
                RxDataBits.push_back(-1);

                mse=0;
                msema->Zero();

                symboltone_averotator=1;
                symboltone_rotator=1;

                rotator=1;
                rotator_freq=0;
                carrier_rotation_est=0;
                st_iir_resonator.init();

                maxvalbin = 0;
                st_osc.SetFreq(st_osc_ref.GetFreqHz());
                st_osc.SetPhaseDeg(0);
                st_osc_ref.SetPhaseDeg(0);
                even = true;
                evenval=0;
                oddval=0;
                lastcntr = 0;


            }
        }//end of trident check


        //sample counting and signalstatus timout
        if(startstop>0)
        {

            if(cntr>=(120*SamplesPerSymbol)){
                startstop--;
            }
            if(cntr<1000000)cntr++;

            if(mse<signalthreshold)
            {
                startstop=startstopstart;
            }


        }
        if(startstop==0)
        {
            startstop--;

            emit SignalStatus(false);
            cntr = 0;
            std::cout << debug.toStdString()<< std::flush;

        }

        //        if(startstop > 0 )
        //        {

        // Normal processing
        //          if(cntr > 120*SamplesPerSymbol){

        cval= mixer2.WTCISValue()*(val_to_demod*vol_gain);
        cpx_type sig2 = cpx_type(matchedfilter_re->FIRUpdateAndProcess(cval.real()),matchedfilter_im->FIRUpdateAndProcess(cval.imag()));

        cpx_type org = sig2;

        if(cntr>(120*SamplesPerSymbol) && cntr<endRotation){

            double progress = (double)cntr-SamplesPerSymbol*120;
            double goal = endRotation-SamplesPerSymbol*120;
            progress = progress/goal;

            cpx_type symboltone_pt=sig2*symboltone_rotator*imag;
            double er=std::tanh(symboltone_pt.imag())*(symboltone_pt.real());

            symboltone_rotator=symboltone_rotator*std::exp(imag*er*0.1);
            symboltone_averotator=symboltone_averotator*0.95+0.05*symboltone_rotator;
            symboltone_pt=cpx_type((symboltone_pt.real()),a1.update(symboltone_pt.real()));

            carrier_rotation_est=std::arg(symboltone_averotator);

            //x4 pll
            /*
            double st_err=std::arg((st_osc_quarter.WTCISValue())*std::conj(symboltone_pt));
            st_err*=1.5*(1.0-progress*progress);
            st_osc_quarter.AdvanceFractionOfWave(-(1.0/(2.0*M_PI))*st_err*0.1);
            st_osc.SetPhaseDeg((st_osc_quarter.GetPhaseDeg())*4.0+(360.0*ee));
            */
        }

          sig2*=symboltone_averotator;

          rotator=rotator*std::exp(imag*rotator_freq);
          sig2*=rotator;


        //Measure ebno
        ebnomeasure->Update(std::abs(sig2));

        //send ebno when right time
        if(cntr==1480*SamplesPerSymbol){
            emit EbNoMeasurmentSignal(ebnomeasure->EbNo);

        }

        //AGC
        sig2*=agc2->Update(std::abs(sig2));

        //clipping

        double abval=std::abs(sig2);
        if(abval>2.84)sig2=(2.84/abval)*sig2;


        //normal symbol timer
        double st_diff=delays.update(abval*abval)-(abval*abval);
        double st_d1out=delayt41.update(st_diff);
        double st_d2out=delayt42.update(st_d1out);
        double st_eta=(st_d2out-st_diff)*st_d1out;

        double reson = st_eta;
        st_iir_resonator.update(st_eta);

        if(cntr>((120)*SamplesPerSymbol))
        {
            st_eta=st_iir_resonator.y;

        }
        //st_eta=st_iir_resonator.update(st_eta);
        cpx_type st_m1=cpx_type(st_eta,-delayt8.update(st_eta));
        cpx_type st_out=st_osc.WTCISValue()*st_m1;
        double st_angle_error=std::arg(st_out);

        //adjust sybol timing using normal tracking
        if(cntr>((120)*SamplesPerSymbol))//???
        {
            st_osc.IncreseFreqHz(-st_angle_error*0.00000001);//st phase shift
            st_osc.AdvanceFractionOfWave(-st_angle_error*0.1/360.0);
        }

        if(st_osc.GetFreqHz()<(st_osc_ref.GetFreqHz()-0.1))st_osc.SetFreq((st_osc_ref.GetFreqHz()-0.1));
        if(st_osc.GetFreqHz()>(st_osc_ref.GetFreqHz()+0.1))st_osc.SetFreq((st_osc_ref.GetFreqHz()+0.1));



        if(cntr>120*SamplesPerSymbol && cntr < 1000*SamplesPerSymbol){

            debug.append(QString::number(cntr)+";");
            debug.append(QString::number(st_iir_resonator.y)+";");
            debug.append(QString::number(sig2.real())+";");
            debug.append(QString::number(sig2.imag())+";");
            debug.append(QString::number(st_osc.IfHavePassedPoint(ee))+"\r\n");
        }


        //sample times
        if(st_osc.IfHavePassedPoint(ee) && cntr > (120*SamplesPerSymbol))//?? 0.4 0.8 etc
        {

            cpx_type pt_msk=cpx_type(sig2.real(), pt_d.imag());

            //for arm ambiguity resolution. bias calibrated for current settings

            double last = (0.34046+0.4111*ee);
            double first = (st_osc_quarter.GetPhaseDeg())*2.0+(360.0*ee*0.5);
            double twospeed=-4.0*((std::fmod(first,360.0)/360.0)-last);

            bool even=true;
            if(twospeed<0)even=false;
            yui++;yui%=2;
            if(cntr<((188)*SamplesPerSymbol))
            {
                if((even&&yui==1)||(!even&&yui==0))
                {
                    yui++;yui%=2;
                }
            }

            if(!yui){pt_d=sig2;}

            else
            {
                lastcntr = cntr;

                //carrier tracking
                double ct_xt=tanh(sig2.imag())*sig2.real();
                double ct_xt_d=tanh(pt_d.real())*pt_d.imag();
                double ct_ec=ct_xt_d-ct_xt;
                if(ct_ec>M_PI)ct_ec=M_PI;
                if(ct_ec<-M_PI)ct_ec=-M_PI;
                if(ct_ec>M_PI_2)ct_ec=M_PI_2;
                if(ct_ec<-M_PI_2)ct_ec=-M_PI_2;
                if(cntr>(endRotation))
                {
                  rotator=rotator*std::exp(imag*ct_ec*0.1);//correct carrier phase
                  if(cntr>(endRotation))rotator_freq=rotator_freq+ct_ec*0.0001;//correct carrier frequency
                }

                //gui feedback

                if(cntr >= 1480*SamplesPerSymbol && pointbuff_ptr<pointbuff.size()){
                    if(pointbuff_ptr<pointbuff.size())
                    {
                        ASSERTCH(pointbuff,pointbuff_ptr);
                        pointbuff[pointbuff_ptr]=pt_msk*0.75;
                        if(pointbuff_ptr<pointbuff.size())pointbuff_ptr++;
                    }
                    if(scatterpointtype==SPT_constellation&&(pointbuff_ptr==pointbuff.size())){
                        pointbuff_ptr++;
                        emit ScatterPoints(pointbuff);
                    }
                }


                //calc MSE of the points
                if(cntr>((120)*SamplesPerSymbol))
                {
                    double tda=(fabs(pt_msk.real())-1.0);
                    double tdb=(fabs(pt_msk.imag())-1.0);
                    mse=msema->Update((tda*tda)+(tdb*tdb));
                }

                //soft bits
                //-1 --> 0 , 1 --> 255 (-1 means 0 and 1 means 1) sort of
                //there is no packed bits in each byte
                //convert to the strange range for soft

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
                if(RxDataBits.size() >= 12){
                    emit processDemodulatedSoftBits(RxDataBits);
                    RxDataBits.clear();
                }
                even = false;
            }
        }

        sig2_last=sig2;



        st_osc.WTnextFrame();
        st_osc_ref.WTnextFrame();
        st_osc_quarter.WTnextFrame();

        mixer2.WTnextFrame();
        mixer_center.WTnextFrame();

        // }// end of if start stop
    }// end of the hrbuff



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

    //if(mse>signalthreshold)emit SignalStatus(false);
    //else emit SignalStatus(true);

}


