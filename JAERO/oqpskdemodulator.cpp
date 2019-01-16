#include "oqpskdemodulator.h"
#include "gui_classes/qspectrumdisplay.h"

#include <QDebug>
#include <QFile>
#include <QTextStream>

OqpskDemodulator::OqpskDemodulator(QObject *parent)
:   QIODevice(parent)
{
    afc=false;
    sql=false;
    scatterpointtype=SPT_constellation;

    mse=100;

    Fs=48000;
    lockingbw=10500;
    freq_center=8000;
    fb=10500;
    signalthreshold=0.5;//lower is less sensitive
    SamplesPerSymbol=2.0*Fs/fb;

    mixer_center.SetFreq(freq_center,Fs);
    mixer2.SetFreq(freq_center,Fs);

    timer.start();

    spectrumnfft=pow(2,SPECTRUM_FFT_POWER);
    spectrumcycbuff.resize(spectrumnfft);
    spectrumcycbuff_ptr=0;

    bbnfft=pow(2,14);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);

    agc = new AGC(4,Fs);

    ebnomeasure = new OQPSKEbNoMeasure(2*Fs,Fs,fb);//2 second ave, Fs and fb

    marg= new MovingAverage(800);
    dt.setLength(400);

    phasepointbuff.resize(2);
    phasepointbuff_ptr=0;

    pointbuff.resize(300);
    pointbuff_ptr=0;

    msecalc = new MSEcalc(400);

    coarsefreqestimate = new CoarseFreqEstimate(this);
    coarsefreqestimate->setSettings(14,lockingbw,fb,Fs);
    connect(this, SIGNAL(BBOverlapedBuffer(const QVector<cpx_type>&)),coarsefreqestimate,SLOT(ProcessBasebandData(const QVector<cpx_type>&)));
    connect(coarsefreqestimate, SIGNAL(FreqOffsetEstimate(double)),this,SLOT(FreqOffsetEstimateSlot(double)));

    RootRaisedCosine rrc;
    rrc.design(1,55,48000,10500/2);
    fir_re=new FIR(rrc.Points.size());
    fir_im=new FIR(rrc.Points.size());
    for(int i=0;i<rrc.Points.size();i++)
    {
        fir_re->FIRSetPoint(i,rrc.Points.at(i));
        fir_im->FIRSetPoint(i,rrc.Points.at(i));
    }

    //st delays
    double T=48000.0/5250.0;
    delays.setdelay(1);
    delayt41.setdelay(T/4.0);
    delayt42.setdelay(T/4.0);
    delayt8.setdelay(T/8.0);//??? T/4.0 or T/8.0 ??? need to check

    //st 10500hz resonator at 48000fps
    st_iir_resonator.a.resize(3);
    st_iir_resonator.b.resize(3);
    st_iir_resonator.b[0]=0.00032714218939589035;
    st_iir_resonator.b[1]=0;
    st_iir_resonator.b[2]=0.00032714218939589035;
    st_iir_resonator.a[0]=1;
    st_iir_resonator.a[1]=-0.39005299948210803;
    st_iir_resonator.a[2]= 0.99934571562120822;
    st_iir_resonator.init();

    //st osc
    st_osc.SetFreq(10500,48000);
    st_osc_ref.SetFreq(10500,48000);

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

    //just for 8400
    //maybe another type of filter would be better?
    fir_pre = new QJFastFIRFilter(this);
    RootRaisedCosine rrc_pre_imp;
    rrc_pre_imp.design(1,1024,48000,10500/2);//8096,48000,10500/2);
    QVector<kffsamp_t> fir_pre_imp_responce;
    fir_pre_imp_responce.resize(rrc_pre_imp.Points.size());
    for(int i=0;i<rrc_pre_imp.Points.size();i++)
    {
        fir_pre_imp_responce[i].r=rrc_pre_imp.Points.at(i);
        fir_pre_imp_responce[i].i=0;
    }
    fir_pre->setKernel(fir_pre_imp_responce);
    mixer_fir_pre.SetFreq(freq_center,Fs);

}

OqpskDemodulator::~OqpskDemodulator()
{
    delete agc;
    delete fir_re;
    delete fir_im;
    delete msecalc;
    delete marg;
    delete ebnomeasure;
    delete coarsefreqestimate;
}

///Connects a sink devide to the modem for the demodulated data
void OqpskDemodulator::ConnectSinkDevice(QIODevice *_datasinkdevice)
{
    if(!_datasinkdevice)return;
    pdatasinkdevice=_datasinkdevice;
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->open(QIODevice::WriteOnly);//!!!overrides the settings
}

///Disconnects the sink devide from the modem
void OqpskDemodulator::DisconnectSinkDevice()
{
    if(pdatasinkdevice.isNull())return;
    QIODevice *io=pdatasinkdevice.data();
    io->close();
    pdatasinkdevice.clear();
}

void OqpskDemodulator::setAFC(bool state)
{
    afc=state;
}

void OqpskDemodulator::setSQL(bool state)
{
    sql=state;
}

void OqpskDemodulator::setScatterPointType(ScatterPointType type)
{
    scatterpointtype=type;
}

void OqpskDemodulator::invalidatesettings()
{
    Fs=-1;
    fb=-1;
}

void OqpskDemodulator::setSettings(Settings _settings)
{
    if(_settings.Fs!=Fs)emit SampleRateChanged(_settings.Fs);
    Fs=_settings.Fs;
    lockingbw=_settings.lockingbw;
    if(_settings.fb!=fb)emit BitRateChanged(_settings.fb,false);
    fb=_settings.fb;
    freq_center=_settings.freq_center;
    if(freq_center>((Fs/2.0)-(lockingbw/2.0)))freq_center=((Fs/2.0)-(lockingbw/2.0));
    signalthreshold=_settings.signalthreshold;
    SamplesPerSymbol=2.0*Fs/fb;

    bbnfft=pow(2,_settings.coarsefreqest_fft_power);
    bbcycbuff.resize(bbnfft);
    bbcycbuff_ptr=0;
    bbtmpbuff.resize(bbnfft);
    coarsefreqestimate->setSettings(_settings.coarsefreqest_fft_power,2.0*lockingbw/2.0,fb,Fs);

    mixer_center.SetFreq(freq_center,Fs);
    mixer2.SetFreq(freq_center,Fs);

    delete agc;
    agc = new AGC(4,Fs);

    pointbuff.resize(300);
    pointbuff_ptr=0;


    // Reset symbol rate dependent stuff


    if(fir_re) delete fir_re;
    if(fir_im) delete fir_im;

    RootRaisedCosine rrc;
    if(fb==8400)rrc.design(0.6,55,48000,fb/2);
     else rrc.design(1.0,55,48000,fb/2);
    fir_re=new FIR(rrc.Points.size());
    fir_im=new FIR(rrc.Points.size());
    for(int i=0;i<rrc.Points.size();i++)
    {
        fir_re->FIRSetPoint(i,rrc.Points.at(i));
        fir_im->FIRSetPoint(i,rrc.Points.at(i));
    }

    //st delays
    double T=48000.0/(fb/2);
    delays.setdelay(1);
    delayt41.setdelay(T/4.0);
    delayt42.setdelay(T/4.0);
    delayt8.setdelay(T/8.0);//??? T/4.0 or T/8.0 ??? need to check

    //st resonator at 48000fps on symbol rate
    st_iir_resonator.a.resize(3);
    st_iir_resonator.b.resize(3);

    if(fb==8400)
    {

        //about 35Hz bw
        st_iir_resonator.b[0]=0.003911649911247;
        st_iir_resonator.b[1]=0;
        st_iir_resonator.b[2]=-0.003911649911247;
        st_iir_resonator.a[0]=1;
        st_iir_resonator.a[1]=-0.904429295683068;
        st_iir_resonator.a[2]= 0.992176700177507;
        ee = 0.65;//0.55;

        //10Hz bw
        st_iir_resonator.b[0]=0.0012845857864470789;
        st_iir_resonator.b[1]=0;
        st_iir_resonator.b[2]=- 0.0012845857864470789;
        st_iir_resonator.a[0]=1;
        st_iir_resonator.a[1]=-0.90681461999279889;
        st_iir_resonator.a[2]= 0.99743082842710584;
        ee = 0.65;

    }
    else
    {

        st_iir_resonator.b[0]=0.00032714218939589035;
        st_iir_resonator.b[1]=0;
        st_iir_resonator.b[2]=0.00032714218939589035;
        st_iir_resonator.a[0]=1;
        st_iir_resonator.a[1]=-0.39005299948210803;
        st_iir_resonator.a[2]= 0.99934571562120822;

        ee = 0.4;

    }
    st_iir_resonator.init();

    //st osc
    st_osc.SetFreq(fb,48000);
    st_osc_ref.SetFreq(fb,48000);

    //ebno measurement ok at 10.5k not
    ebnomeasure->setup_update(Fs,fb);

    //the rrc filter for 8400bps probably could be used for 10500 but the 10500 works well enough.
    //maybe another type of filter would be better?
    RootRaisedCosine rrc_pre_imp;
    if(fb==8400)rrc_pre_imp.design(0.6,2048,Fs,fb/2);//0.6 --> smaller number mean less interchannel interference but locking is harder
     else rrc_pre_imp.design(1.0,2048,Fs,fb/2);
    QVector<kffsamp_t> fir_pre_imp_responce;
    fir_pre_imp_responce.resize(rrc_pre_imp.Points.size());
    for(int i=0;i<rrc_pre_imp.Points.size();i++)
    {
        fir_pre_imp_responce[i].r=rrc_pre_imp.Points.at(i);
        fir_pre_imp_responce[i].i=0;
    }
    fir_pre->setKernel(fir_pre_imp_responce,4096);//use x2 rather than the x4 rule of thumb, will make it more responsive but may use more cpu

    emit Plottables(mixer2.GetFreqHz(),mixer_center.GetFreqHz(),lockingbw);
}

void OqpskDemodulator::CenterFreqChangedSlot(double freq_center)//spectrum display calls this when user changes the center freq
{
    if(fb!=8400)//ppl are going to shift the signal too far left or right as alpha<1.0
    {
        if(freq_center<(0.5*fb))freq_center=0.5*fb;
        if(freq_center>(Fs/2.0-0.5*fb))freq_center=Fs/2.0-0.5*fb;
    }
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

void OqpskDemodulator::start()
{
    open(QIODevice::WriteOnly);
}

void OqpskDemodulator::stop()
{
    close();
}

double  OqpskDemodulator::getCurrentFreq()
{
    return mixer_center.GetFreqHz();
}

qint64 OqpskDemodulator::readData(char *data, qint64 maxlen)
{
    Q_UNUSED(data);
    Q_UNUSED(maxlen);
    return 0;
}

qint64 OqpskDemodulator::writeData(const char *data, qint64 len)
{

    if(!len)return 0;

    double lastmse=mse;

    bool sendscatterpoints=false;

    //prefilter start
    QVector<cpx_type> cval_prefiltered;
    if(fb==8400)
    {

        //I think the org estimate good enough so dont bother
        //mixer_fir_pre.SetFreq(mixer2.GetFreqHz()*0.03+0.97*mixer_fir_pre.GetFreqHz());

        //down and convert to kiss vector
        QVector<kffsamp_t> cval_tmp;
        cval_tmp.resize(len/sizeof(short));
        const short *ptr2 = reinterpret_cast<const short *>(data);
        double savedphase=mixer_fir_pre.GetPhaseDeg();
        for(int i=0;i<len/sizeof(short);i++)
        {
            double dval=((double)(*ptr2))/32768.0;

            //down
            cpx_type cval=mixer_fir_pre.WTCISValue()*dval;

            //save as kiss
            kffsamp_t cval_kiss;
            cval_kiss.r=cval.real();
            cval_kiss.i=cval.imag();
            cval_tmp[i]=cval_kiss;

            //next
            mixer_fir_pre.WTnextFrame();
            ptr2++;
        }

        //filter kiss vector
        fir_pre->Update(cval_tmp);

        //up and convert to std::complex vector
        cval_prefiltered.resize(cval_tmp.size());
        mixer_fir_pre.SetPhaseDeg(savedphase);
        for(int i=0;i<cval_tmp.size();i++)
        {
            //up
            cpx_type cval=cpx_type(cval_tmp[i].r,cval_tmp[i].i);
            cval=mixer_fir_pre.WTCISValue_conj()*cval;

            //save as std::complex
            cval_prefiltered[i]=cval;

            //next
            mixer_fir_pre.WTnextFrame();
        }

        //clear junk
        cval_tmp.clear();
    }

    //prefilter end

    double mixer2_freq_sum=0;
    int i=0;
    const short *ptr = reinterpret_cast<const short *>(data);
    for(i=0;i<len/sizeof(short);i++)
    {
        double dval=((double)(*ptr))/32768.0;

        //for looks
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

        //for coarse freq estimation
        ASSERTCH(bbcycbuff,bbcycbuff_ptr);
        bbcycbuff[bbcycbuff_ptr]=mixer_center.WTCISValue()*dval;
        bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
        if(bbcycbuff_ptr%(bbnfft/4)==0)//75% overlap
        {
            for(int j=0;j<bbcycbuff.size();j++)
            {
                ASSERTCH(bbcycbuff,bbcycbuff_ptr);
                ASSERTCH(bbtmpbuff,j);
                bbtmpbuff[j]=bbcycbuff[bbcycbuff_ptr];
                bbcycbuff_ptr++;bbcycbuff_ptr%=bbnfft;
            }
            emit BBOverlapedBuffer(bbtmpbuff);
        }

        //-----

        cpx_type cval,sig2;

        if(fb==8400)
        {
            //already had a good enough rrc filter just needs mixing down
            //mix
            sig2= mixer2.WTCISValue()*cval_prefiltered[i];

            //this would be both
            //cval=mixer2.WTCISValue()*cval_prefiltered[i];
            //sig2=cpx_type(fir_re->FIRUpdateAndProcess(cval.real()),fir_im->FIRUpdateAndProcess(cval.imag()));

            //calc ave of freq over this block for 8400bps prefilter
            mixer2_freq_sum+=mixer2.GetFreqHz();

        }
         else
         {
            //mix
            cval= mixer2.WTCISValue()*dval;

            //rrc
            sig2=cpx_type(fir_re->FIRUpdateAndProcess(cval.real()),fir_im->FIRUpdateAndProcess(cval.imag()));
         }

        //Measure ebno
        ebnomeasure->Update(std::abs(sig2));

        //AGC
        sig2*=agc->Update(std::abs(sig2));

        //clipping
        double abval=std::abs(sig2);
        if(abval>2.84)sig2=(2.84/abval)*sig2;

        //symbol timer
        double st_diff=delays.update(abval*abval)-(abval*abval);
        double st_d1out=delayt41.update(st_diff);
        double st_d2out=delayt42.update(st_d1out);
        double st_eta=(st_d2out-st_diff)*st_d1out;
        st_eta=st_iir_resonator.update(st_eta);
        cpx_type st_m1=cpx_type(st_eta,-delayt8.update(st_eta));
        cpx_type st_out=st_osc.WTCISValue()*st_m1;
        double st_angle_error=std::arg(st_out);
        st_osc.IncreseFreqHz(-st_angle_error*0.00000001);//st phase shift
        st_osc.AdvanceFractionOfWave(-st_angle_error*0.01/360.0);
        if(st_osc.GetFreqHz()<(st_osc_ref.GetFreqHz()-0.1))st_osc.SetFreq((st_osc_ref.GetFreqHz()-0.1));
        if(st_osc.GetFreqHz()>(st_osc_ref.GetFreqHz()+0.1))st_osc.SetFreq((st_osc_ref.GetFreqHz()+0.1));

        //sample times
        static cpx_type sig2_last=sig2;
        if(st_osc.IfHavePassedPoint(ee))
        {

            //interpol
            double pt_last=st_osc.FractionOfSampleItPassesBy;
            double pt_this=1.0-pt_last;
            cpx_type pt=pt_this*sig2+pt_last*sig2_last;

            static int yui=0;
            yui++;yui%=2;
            static cpx_type pt_d=0;
            if(!yui)pt_d=pt;
             else
            {

                cpx_type pt_qpsk=cpx_type(pt.real(),pt_d.imag());

                //4 power method
                //cpx_type trick_pt=std::pow(pt_qpsk*cpx_type(cos(M_PI_4),sin(M_PI_4)),4);
                //double fang=std::arg(trick_pt)/4.0;
                //fang=std::arg(eightpt)/8.0;
                //mixer2.IncresePhaseDeg(-2.0*fang/2.0);
                //mixer2.IncreseFreqHz(-fang/20.0);

                //carrier tracking BPSK 2x method
                double ct_xt=tanh(pt.imag())*pt.real();
                double ct_xt_d=tanh(pt_d.real())*pt_d.imag();
                double ct_ec=ct_xt_d-ct_xt;
                if(ct_ec>M_PI)ct_ec=M_PI;
                if(ct_ec<-M_PI)ct_ec=-M_PI;
                if(fb>8400)
                {
                  ct_ec=ct_iir_loopfilter.update(ct_ec);
                  if(ct_ec>M_PI_2)ct_ec=M_PI_2;
                  if(ct_ec<-M_PI_2)ct_ec=-M_PI_2;
                  mixer2.IncresePhaseDeg(1.0*ct_ec);//*cos(ct_ec));
                  mixer2.IncreseFreqHz(0.01*ct_ec);//*(1.0-cos(ct_ec)));
                }
                 else
                 {
                    //8400 works better with faster phase agility
                    mixer2.IncresePhaseDeg(1.0*ct_ec);
                    mixer2.IncreseFreqHz(0.5*0.01*ct_iir_loopfilter.update(ct_ec));
                    //mixer2.IncreseFreqHz(0.5*0.01*ct_ec);
                 }

                //rotate to remove any remaining bias
                marg->UpdateSigned(ct_ec);
                dt.update(pt_qpsk);
                pt_qpsk*=cpx_type(cos(marg->Val),sin(marg->Val));

                //gui feedback
                static int slowdown=0;
                if(pointbuff_ptr==0){slowdown++;slowdown%=100;}
                //for looks show constellation
                if(!slowdown)
                {
                    ASSERTCH(pointbuff,pointbuff_ptr);
                    pointbuff[pointbuff_ptr]=pt_qpsk;
                    pointbuff_ptr++;pointbuff_ptr%=pointbuff.size();
                    //if(scatterpointtype==SPT_constellation&&(pointbuff_ptr%100==0))emit ScatterPoints(pointbuff);
                    if(scatterpointtype==SPT_constellation&&sendscatterpoints){sendscatterpoints=false;emit ScatterPoints(pointbuff);}
                }
                //for looks show symbol phase offset
                if(!slowdown)
                {
                    cpx_type st_phase_offset_pt=st_osc_ref.WTCISValue()*std::conj(st_osc.WTCISValue());
                    ASSERTCH(phasepointbuff,phasepointbuff_ptr);
                    phasepointbuff[phasepointbuff_ptr]=st_phase_offset_pt;
                    phasepointbuff_ptr++;phasepointbuff_ptr%=phasepointbuff.size();
                    //if(scatterpointtype==SPT_phaseoffsetest)emit ScatterPoints(phasepointbuff);
                    if(scatterpointtype==SPT_phaseoffsetest&&sendscatterpoints){sendscatterpoints=false;emit ScatterPoints(phasepointbuff);}
                }

                //calc MSE of the points
                mse=msecalc->Update(pt_qpsk);

                /*
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
                }*/

                // soft bits
                int ibit=qRound(0.75*pt_qpsk.imag()*127.0+128.0);
                if(ibit>255)ibit=255;
                if(ibit<0)ibit=0;

                RxDataBits.push_back((uchar)ibit);

                ibit=qRound(0.75*pt_qpsk.real()*127.0+128.0);
                if(ibit>255)ibit=255;
                if(ibit<0)ibit=0;

                RxDataBits.push_back((uchar)ibit);

                //return the demodulated data (soft bit)

                if(RxDataBits.size() >= 32)
                {
                    if(!sql||mse<signalthreshold||lastmse<signalthreshold)
                    {

                        emit processDemodulatedSoftBits(RxDataBits);

                    }
                    RxDataBits.clear();
                }
            }
        }
        sig2_last=sig2;

        //-----

        mixer2.WTnextFrame();
        mixer_center.WTnextFrame();
        st_osc.WTnextFrame();
        st_osc_ref.WTnextFrame();
        ptr++;
    }

    //update the 8400bps pre filter with better estimates of carrier in case someone uses C band with lots of drift. untested on C-band.
    mixer_fir_pre.SetFreq(mixer2_freq_sum/((double)i));

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

    return len;

}

void OqpskDemodulator::FreqOffsetEstimateSlot(double freq_offset_est)//coarse est class calls this with current est
{
  //  qDebug()<<freq_offset_est;

    //update prefilter untill we have dcd and a signal
    if((mse>signalthreshold)||(!dcd))
    {
        //qDebug()<<mixer_center.GetFreqHz()+freq_offset_est;
        mixer_fir_pre.SetFreq(mixer_center.GetFreqHz()+freq_offset_est,Fs);
    }

    //for preventing bad stable states
    static int countdown2=5;
    if((mse<signalthreshold)&&(!dcd))//signal but we arent getting data so prob in a state that is stable but wrong
    {
        if(countdown2>0)countdown2--;
         else
         {
            //qDebug()<<"resetting";
            mixer2.SetFreq(mixer_center.GetFreqHz()+freq_offset_est);
         }
    } else countdown2=5;

    static int countdown=4;
    if((mse>signalthreshold)&&(fabs(mixer2.GetFreqHz()-(mixer_center.GetFreqHz()+freq_offset_est))>3.0))//no sig, prob cant track carrier phase
    {
        mixer2.SetFreq(mixer_center.GetFreqHz()+freq_offset_est);
        //symtracker.Reset();
    }
    if((afc)&&(mse<signalthreshold)&&(fabs(mixer2.GetFreqHz()-mixer_center.GetFreqHz())>3.0))//got a sig, afc on, freq is getting a little to far out
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

void OqpskDemodulator::DCDstatSlot(bool _dcd)
{

    dcd=_dcd;

}
