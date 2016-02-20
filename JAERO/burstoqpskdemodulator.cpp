#include "burstoqpskdemodulator.h"
#include "gui_classes/qspectrumdisplay.h"

BurstOqpskDemodulator::BurstOqpskDemodulator(QObject *parent)
    :   QIODevice(parent)
{

    timer.start();

    spectrumnfft=pow(2,SPECTRUM_FFT_POWER);
    spectrumcycbuff.resize(spectrumnfft);
    spectrumcycbuff_ptr=0;

    mse=100;

    insertpreamble=false;

    bbnfft=pow(2,14);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);

    Fs=48000;
    fb=10500;
    SamplesPerSymbol=2.0*Fs/fb;
    agc = new AGC(0.05,Fs);

    agc2 = new AGC(50.0/Fs,Fs);

    hfir=new QJHilbertFilter(this);

    fftr = new FFTr(pow(2.0,(ceil(log2(  128.0*SamplesPerSymbol   )))),false);
    mav1= new MovingAverage(SamplesPerSymbol*128);

    rotation_bias_delay.setLength(256);
    rotation_bias_ma = new MovingAverage(256);

  //--demod (resonators and LPF hard coded for Fs==48000 and fb==10500)

    RootRaisedCosine rrc;
    rrc.design(1,55,Fs,fb/2.0);
    fir_re=new FIR(rrc.Points.size());
    fir_im=new FIR(rrc.Points.size());
    for(int i=0;i<rrc.Points.size();i++)
    {
        fir_re->FIRSetPoint(i,rrc.Points.at(i));
        fir_im->FIRSetPoint(i,rrc.Points.at(i));
    }

    //st delays
    delays.setdelay(1);
    delayt41.setdelay(SamplesPerSymbol/4.0);
    delayt42.setdelay(SamplesPerSymbol/4.0);
    delayt8.setdelay(SamplesPerSymbol/8.0);//??

    //st 10500hz resonator at 48000fps
    st_iir_resonator.a.resize(3);
    st_iir_resonator.b.resize(3);

    //5hz
    /*st_iir_resonator.b[0]=0.00032714218939589035;
    st_iir_resonator.b[1]=0;
    st_iir_resonator.b[2]=0.00032714218939589035;
    st_iir_resonator.a[0]=1;
    st_iir_resonator.a[1]=-0.39005299948210803;
    st_iir_resonator.a[2]= 0.99934571562120822;
    st_iir_resonator.init();*/

    //75hz
    st_iir_resonator.b[0]=0.0048847995518126464;
    st_iir_resonator.b[1]=0;
    st_iir_resonator.b[2]=-0.0048847995518126464;
    st_iir_resonator.a[0]=1;
    st_iir_resonator.a[1]=-0.3882746897971619;
    st_iir_resonator.a[2]=0.99023040089637471;
    st_iir_resonator.init();


    //st osc
    st_osc.SetFreq(fb,Fs);
    st_osc_ref.SetFreq(fb,Fs);
    st_osc_quarter.SetFreq(fb/4.0,Fs);

    //ct LPF
    ct_iir_loopfilter.a.resize(3);
    ct_iir_loopfilter.b.resize(3);
    ct_iir_loopfilter.b[0]=0.0010275610653672064;//0.0109734835527828;
    ct_iir_loopfilter.b[1]=0.0020551221307344128;//0.0219469671055655;
    ct_iir_loopfilter.b[2]=0.0010275610653672064;//0.0109734835527828;
    ct_iir_loopfilter.a[0]=1;
    ct_iir_loopfilter.a[1]=-1.9207386815577139  ;//-1.72040443455951;
    ct_iir_loopfilter.a[2]=  0.92509247310306331 ;//0.766899247885339;
    ct_iir_loopfilter.init();

    //BC
    bc.SetInNumberOfBits(1);
    bc.SetOutNumberOfBits(8);

    //rxdata
    RxDataBytes.reserve(1000);//packed in bytes

    ebnomeasure = new OQPSKEbNoMeasure(SamplesPerSymbol*(256.0),Fs,fb);//256 symbol ave, Fs and fb

    msema = new MovingAverage(128);

    phasepointbuff.resize(1);
    phasepointbuff_ptr=0;


 //--


    Settings _settings;
    invalidatesettings();
    setSettings(_settings);


}

BurstOqpskDemodulator::~BurstOqpskDemodulator()
{
    delete agc;
    delete agc2;
    delete mav1;
    delete fftr;
    delete fir_re;
    delete fir_im;
    delete ebnomeasure;
    delete msema;
    delete rotation_bias_ma;
}

///Connects a sink devide to the modem for the demodulated data
void BurstOqpskDemodulator::ConnectSinkDevice(QIODevice *_datasinkdevice)
{
    if(!_datasinkdevice)return;
    pdatasinkdevice=_datasinkdevice;
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->open(QIODevice::WriteOnly);//!!!overrides the settings
}

///Disconnects the sink devide from the modem
void BurstOqpskDemodulator::DisconnectSinkDevice()
{
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->close();
    pdatasinkdevice.clear();
}

void BurstOqpskDemodulator::setAFC(bool state)
{
    afc=state;
}

void BurstOqpskDemodulator::setSQL(bool state)
{
    sql=state;
}

void BurstOqpskDemodulator::setScatterPointType(ScatterPointType type)
{
    scatterpointtype=type;
}

void BurstOqpskDemodulator::invalidatesettings()
{
    Fs=-1;
    fb=-1;
}

void BurstOqpskDemodulator::start()
{
    open(QIODevice::WriteOnly);
}

void BurstOqpskDemodulator::stop()
{
    close();
}

void BurstOqpskDemodulator::setSettings(Settings _settings)
{
    if(_settings.Fs!=Fs)emit SampleRateChanged(_settings.Fs);
    Fs=_settings.Fs;
    lockingbw=_settings.lockingbw;
    if(_settings.fb!=fb)emit BitRateChanged(_settings.fb,true);
    fb=_settings.fb;
    freq_center=_settings.freq_center;
    if(freq_center>((Fs/2.0)-(lockingbw/2.0)))freq_center=((Fs/2.0)-(lockingbw/2.0));
    signalthreshold=_settings.signalthreshold;
    SamplesPerSymbol=2.0*Fs/fb;

    bbnfft=pow(2,_settings.coarsefreqest_fft_power);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);

    mixer2.SetFreq(freq_center,Fs);

    delete agc;
    agc = new AGC(1,Fs);

    delete agc2;
    agc2 = new AGC(SamplesPerSymbol*64.0/Fs,Fs);

    hfir->setSize(2048);

    pointbuff.resize(128);
    pointbuff_ptr=0;

    bt_d1.setdelay(1.0*SamplesPerSymbol);
    bt_ma1.setLength(qRound(128.0*SamplesPerSymbol));//not sure whats best

    delete mav1;
    mav1= new MovingAverage(SamplesPerSymbol*128);

    bt_ma_diff.setdelay(SamplesPerSymbol*128);//not sure whats best

    d1.setLength((int)(SamplesPerSymbol*128.0*2.5-190));//adjust so start of burst aligns

    delete fftr;
    int N=4096*4*2;
    fftr = new FFTr(N,false);

    tridentbuffer_sz=qRound((256.0+16.0+16.0)*SamplesPerSymbol);//room for trident and preamble and a bit more
    tridentbuffer.resize(tridentbuffer_sz);
    tridentbuffer_ptr=0;

    d2.setLength(tridentbuffer_sz);//delay line for aligning demod to output of trident check and freq est

    in.resize(N);
    out_base.resize(N);
    out_top.resize(N);
    out_abs_diff.resize(N/2);

    pdet.setSettings((int)(SamplesPerSymbol*128.0/2.0),0.2);

    a1.setdelay(SamplesPerSymbol/2.0);
    ee=0.4;//0.4;
    symboltone_averotator=1;
    carrier_rotation_est=0;

    delete ebnomeasure;
    ebnomeasure = new OQPSKEbNoMeasure(SamplesPerSymbol*(256.0),Fs,fb);//256 symbol ave, Fs and fb

    rotator=1;

    startstopstart=SamplesPerSymbol*(1050);//(256+2000);//128);

    insertpreamble=false;

    emit Plottables(mixer2.GetFreqHz(),mixer2.GetFreqHz(),lockingbw);

}

double  BurstOqpskDemodulator::getCurrentFreq()
{
    return mixer2.GetFreqHz();
}

void BurstOqpskDemodulator::CenterFreqChangedSlot(double freq_center)//spectrum display calls this when user changes the center freq
{
    Q_UNUSED(freq_center);
    //nothing
    return;
}

qint64 BurstOqpskDemodulator::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}

qint64 BurstOqpskDemodulator::writeData(const char *data, qint64 len)
{
    if(len<1)return len;
    double lastmse=mse;
    bool sendscatterpoints=false;
    if(len<sizeof(short))return len;


    //make analytical signal
    hfirbuff.resize(len/sizeof(short));
    kffsamp_t asample;
    asample.i=0;
    const short *ptr = reinterpret_cast<const short *>(data);
    for(int i=0;i<len/sizeof(short);i++)
    {
        //double dval=((double)(*ptr))/32768.0;
        //agc->Update(dval);
        asample.r=((double)(*ptr))/32768.0;//agc->Update(((double)(*ptr))/32768.0);
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
        static double maxval=0;
        if(fabs(dval)>maxval)maxval=fabs(dval);
        spectrumcycbuff[spectrumcycbuff_ptr]=dval;
        spectrumcycbuff_ptr++;spectrumcycbuff_ptr%=spectrumnfft;
        if(timer.elapsed()>150)
        {
            sendscatterpoints=true;
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
        double val_to_demod=(d2.update_dont_touch(std::real(cval_d)));

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

        static int startstop=-1;
        static double vol_gain=1;
        static int cntr=0;

        if(tridentbuffer_ptr<tridentbuffer_sz)//fill trident buffer
        {
            tridentbuffer[tridentbuffer_ptr]=std::real(cval_d);
            tridentbuffer_ptr++;
        }
         else if(tridentbuffer_ptr==tridentbuffer_sz)//trident buffer is now filled so now check for trident and carrier freq and phase and amplitude
         {
            tridentbuffer_ptr++;

            //base
            in=tridentbuffer.mid(0,qRound(128.0*SamplesPerSymbol));
            in.resize(out_base.size());
            for(int k=qRound(128.0*SamplesPerSymbol);k<in.size();k++)in[k]=0;
            fftr->transform(in,out_base);

            //top
            in=tridentbuffer.mid(qRound(128.0*SamplesPerSymbol),qRound(128.0*SamplesPerSymbol));
            in.resize(out_top.size());
            for(int k=qRound(128.0*SamplesPerSymbol);k<in.size();k++)in[k]=0;
            fftr->transform(in,out_top);

            //diff
            for(int i=0;i<out_abs_diff.size();i++)out_abs_diff[i]=(std::abs(out_top[i])-std::abs(out_base[i]));

            //find best trident loc
            double hzperbin=Fs/((double)out_base.size());
            double binpeakspacing=(0.25*fb)/hzperbin;
            int binpeakspacing_int=qRound(binpeakspacing);
            int firstbin=binpeakspacing_int;
            int lstbin=out_base.size()/2-binpeakspacing_int;
            double maxval=out_abs_diff[firstbin-binpeakspacing_int]+out_abs_diff[firstbin+binpeakspacing_int]-out_abs_diff[firstbin];
            double maxvalbin=firstbin;
            for(int i=firstbin;i<lstbin;i++)
            {
                assert(i<out_abs_diff.size());
                double testval=out_abs_diff[i-binpeakspacing_int]+out_abs_diff[i+binpeakspacing_int]-out_abs_diff[i];
                if(testval>maxval)
                {
                    maxval=testval;
                    maxvalbin=i;
                }
            }

            //find strongest base loc
            double minval2=std::abs(out_top[0])-std::abs(out_base[0]);
            //double minvalbin2=0;
            for(int i=0;i<out_base.size()/2;i++)
            {
                if((std::abs(out_top[i])-std::abs(out_base[i]))<minval2)
                {
                    minval2=std::abs(out_top[i])-std::abs(out_base[i]);
                    //minvalbin2=i;
                }
            }

            //find strongest base loc
            double minval=std::abs(out_base[0]);
            double minvalbin=0;
            for(int i=0;i<out_base.size()/2;i++)
            {
                if((std::abs(out_base[i]))>minval)
                {
                    minval=std::abs(out_base[i]);
                    minvalbin=i;
                }
            }

            //check that signal is strong enough and two locs are close else this is not a signal
            if((maxval>500.0)&&(fabs((((double)(maxvalbin-minvalbin)))*hzperbin)<20.0))
            {

                //qDebug()<<"vaxval="<<maxval<<" maxvalhz1="<<hzperbin*maxvalbin<<" maxvalhz2="<<hzperbin*minvalbin<<"Hz"<<" maxvalhz2b="<<hzperbin*minvalbin2<<"Hz";

                //set the demodulator carrier freq and phase using estimates
                double carrierphase=std::arg(out_base[minvalbin])-(M_PI/4.0);
                mixer2.SetFreq(hzperbin*minvalbin);
                mixer2.SetPhaseDeg((180.0/M_PI)*carrierphase);
                emit Plottables(mixer2.GetFreqHz(),mixer2.GetFreqHz(),lockingbw);

                //set gain given estimate
                vol_gain=1.4142*500.0/minval;

                //set when we want to store points for display
                //if using rotation bias correction
                //pointbuff_ptr=-128-128-256;//-128-128;//-400;//-500;//-100;
                //else
                pointbuff_ptr=-128-128;

                //reset the rest
                st_osc.SetFreq(st_osc_ref.GetFreqHz());
                st_osc.SetPhaseDeg(0);
                st_osc_ref.SetPhaseDeg(0);
                st_iir_resonator.init();
                ct_iir_loopfilter.init();
                pointbuff.fill(0);
                startstop=startstopstart;
                cntr=0;
                rotator=1;
                insertpreamble=true;
                rotator_freq=0;
                symboltone_averotator=1;
                carrier_rotation_est=0;
                //qDebug()<<"start";
                emit SignalStatus(true);
                mse=0;
                msema->Zero();

            }


         }//end of trident check

        //mix
        cpx_type cval_dd=mixer2.WTCISValue()*(vol_gain*val_to_demod);

        //rrc
        cpx_type sig2=cpx_type(fir_re->FIRUpdateAndProcess(cval_dd.real()),fir_im->FIRUpdateAndProcess(cval_dd.imag()));

        //sample counting and signalstatus timout
        if(startstop>0)
        {
            startstop--;
            if(cntr<1000000)cntr++;
            if(mse<0.75)
            {
                startstop=startstopstart;
            }


        }
        if(startstop==0)
        {
            startstop--;
//            qDebug()<<"stop";
            emit SignalStatus(false);
        }

        if((cntr>((256-10)*SamplesPerSymbol))&&insertpreamble)
        {
            insertpreamble=false;
            RxDataBytes.push_back((uchar)0x11);
            RxDataBytes.push_back((uchar)0x07);
            RxDataBytes.push_back((uchar)0x42);
            RxDataBytes.push_back((char)0x00);
            RxDataBytes.push_back((char)0x00);
            RxDataBytes.push_back((uchar)0x13);
            RxDataBytes.push_back((uchar)0x09);
//            qDebug()<<"UW almost here";
        }

        //symbol tone in preamble
        if((cntr>SamplesPerSymbol*(128+10))&&(cntr<((256-10)*SamplesPerSymbol)))
        {

            //0 to 1
            double progress=(((double)cntr)-(SamplesPerSymbol*(128+10)))/(((256-10)*SamplesPerSymbol)-(SamplesPerSymbol*(128+10)));

            //produce symbol tone circle (symboltone_pt) and calc carrier rotation
            static cpx_type symboltone_rotator=1;
            cpx_type symboltone_pt=sig2*symboltone_rotator*imag;
            double er=std::tanh(symboltone_pt.imag())*(symboltone_pt.real());
            symboltone_rotator=symboltone_rotator*std::exp(imag*er*0.01);
            symboltone_averotator=symboltone_averotator*0.95+0.05*symboltone_rotator;
            symboltone_pt=cpx_type((symboltone_pt.real()),a1.update(symboltone_pt.real()));
            carrier_rotation_est=std::arg(symboltone_averotator);

            //x4 pll
            double st_err=std::arg((st_osc_quarter.WTCISValue())*std::conj(symboltone_pt));
            st_err*=1.5*(1.0-progress*progress);
            st_osc_quarter.AdvanceFractionOfWave(-(1.0/(2.0*M_PI))*st_err*0.1);
            st_osc.SetPhaseDeg((st_osc_quarter.GetPhaseDeg())*4.0+(360.0*ee));

        }

        //correct carrier phase
        sig2*=symboltone_averotator;
        rotator=rotator*std::exp(imag*rotator_freq);
        sig2*=rotator;

        //Measure ebno
        ebnomeasure->Update(std::abs(sig2));

        //send ebno when right time
        if(fabs(cntr-((128.0+128.0+128.0)*SamplesPerSymbol))<0.5)emit EbNoMeasurmentSignal(ebnomeasure->EbNo);

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
        st_iir_resonator.update(st_eta);
        if(cntr>SamplesPerSymbol*(128+128))st_eta=st_iir_resonator.y;
        //st_eta=st_iir_resonator.update(st_eta);
        cpx_type st_m1=cpx_type(st_eta,-delayt8.update(st_eta));
        cpx_type st_out=st_osc.WTCISValue()*st_m1;
        double st_angle_error=std::arg(st_out);

        //adjust sybol timing using normal tracking
        if(cntr>SamplesPerSymbol*(128+64))//???
        {
            st_osc.IncreseFreqHz(-st_angle_error*0.00000001);//st phase shift
            st_osc.AdvanceFractionOfWave(-st_angle_error*0.01/360.0);
        }
        if(st_osc.GetFreqHz()<(st_osc_ref.GetFreqHz()-0.1))st_osc.SetFreq((st_osc_ref.GetFreqHz()-0.1));
        if(st_osc.GetFreqHz()>(st_osc_ref.GetFreqHz()+0.1))st_osc.SetFreq((st_osc_ref.GetFreqHz()+0.1));

        //sample times
        static cpx_type sig2_last=sig2;
        if(st_osc.IfHavePassedPoint(ee))//?? 0.4 0.8 etc
        {

            //interpol
            double pt_last=st_osc.FractionOfSampleItPassesBy;
            double pt_this=1.0-pt_last;
            cpx_type pt=pt_this*sig2+pt_last*sig2_last;

            //for arm ambiguity resolution. bias calibrated for current settings
            double twospeed=-4.0*((std::fmod((st_osc_quarter.GetPhaseDeg())*2.0+(360.0*ee*0.5),360.0)/360.0)-(0.34046+0.4111*ee));
            bool even=true;
            if(twospeed<0)even=false;
            static int yui=0;
            yui++;yui%=2;
            if(cntr<((128+128)*SamplesPerSymbol))
            {
                if((even&&yui==1)||(!even&&yui==0))
                {
                    yui++;yui%=2;
                }
            }

            static cpx_type pt_d=0;
            if(!yui)pt_d=pt;
             else
            {

                cpx_type pt_qpsk=cpx_type(pt.real(),pt_d.imag());

                //carrier tracking BPSK 2x method
                double ct_xt=tanh(pt.imag())*pt.real();
                double ct_xt_d=tanh(pt_d.real())*pt_d.imag();
                double ct_ec=ct_xt_d-ct_xt;
                if(ct_ec>M_PI)ct_ec=M_PI;
                if(ct_ec<-M_PI)ct_ec=-M_PI;
                //ct_ec=ct_iir_loopfilter.update(ct_ec);
                if(ct_ec>M_PI_2)ct_ec=M_PI_2;
                if(ct_ec<-M_PI_2)ct_ec=-M_PI_2;

                if(cntr>((128+10)*SamplesPerSymbol))//???
                {
                    rotator=rotator*std::exp(imag*ct_ec*0.1);//correct carrier phase
                    if(cntr>((128+10)*SamplesPerSymbol))rotator_freq=rotator_freq+ct_ec*0.0001;//correct carrier frequency
                }

                //rotation bias correction using 4 power method
                /*cpx_type trick_pt=std::pow(pt_qpsk*cpx_type(cos(M_PI_4),sin(M_PI_4)),4);
                double fang=std::arg(trick_pt)/4.0;
                rotation_bias_ma->UpdateSigned(fang);
                rotation_bias_delay.update(pt_qpsk);
                pt_qpsk*=std::exp(-imag*rotation_bias_ma->Val);*/

                //gui feedback
                if(pointbuff_ptr>0)
                {
                    if(pointbuff_ptr<pointbuff.size())
                    {
                        ASSERTCH(pointbuff,pointbuff_ptr);
                        pointbuff[pointbuff_ptr]=pt_qpsk;
                        if(pointbuff_ptr<pointbuff.size())pointbuff_ptr++;
                    }
                    if(scatterpointtype==SPT_constellation&&(pointbuff_ptr==pointbuff.size())&&(sendscatterpoints)){pointbuff_ptr++;emit ScatterPoints(pointbuff);}
                } else pointbuff_ptr++;

                //for looks show symbol phase offset. whats the point?
                if(scatterpointtype==SPT_phaseoffsetest)
                {
                    if(pointbuff_ptr>pointbuff.size())pointbuff_ptr=pointbuff.size();
                    if(fabs(cntr-((128.0+128.0+128.0)*SamplesPerSymbol))<SamplesPerSymbol)
                    {
                        cpx_type st_phase_offset_pt=st_osc_ref.WTCISValue()*std::conj(st_osc.WTCISValue());
                        ASSERTCH(phasepointbuff,phasepointbuff_ptr);
                        phasepointbuff[phasepointbuff_ptr]=st_phase_offset_pt;
                        phasepointbuff_ptr++;phasepointbuff_ptr%=phasepointbuff.size();
                        emit ScatterPoints(phasepointbuff);
                    }
                }

                //calc MSE of the points
                if(cntr>((128+10)*SamplesPerSymbol))
                {
                    double tda=(fabs(pt_qpsk.real())-1.0);
                    double tdb=(fabs(pt_qpsk.imag())-1.0);
                    mse=msema->Update((tda*tda)+(tdb*tdb));
                }


                //if(startstop>0)//if signal then may as well demodulate
                {

                    //hard BPSK demod x2
                    bool pt_qpsk_imag_demod=0;
                    bool pt_qpsk_real_demod=0;
                    if(pt_qpsk.imag()>0)pt_qpsk_imag_demod=1;
                    if(pt_qpsk.real()>0)pt_qpsk_real_demod=1;

                    //if you want packed bits
                    bc.LoadSymbol(pt_qpsk_imag_demod);
                    bc.LoadSymbol(pt_qpsk_real_demod);
                    while(bc.DataAvailable)
                    {
                        bc.GetNextSymbol();
                        RxDataBytes.push_back((uchar)bc.Result);
                    }

                    //----
                    //return the demodulated data (packed in bytes)
                    //using bytes and the qiodevice class
                    if(RxDataBytes.size()>256)
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

                }


            }


        }
        sig2_last=sig2;

        mixer2.WTnextFrame();
        st_osc.WTnextFrame();
        st_osc_ref.WTnextFrame();
        st_osc_quarter.WTnextFrame();

    }

    //return the demodulated data (packed in bytes)
    //using bytes and the qiodevice class
    /*if(!RxDataBytes.isEmpty())
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
    }*/

    return len;
}
