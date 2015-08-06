#include "coarsefreqestimate.h"
#include <assert.h>
#include <QDebug>

CoarseFreqEstimate::CoarseFreqEstimate(QObject *parent) : QObject(parent)
{
    coarsefreqest_fft_power=13;
    lockingbw=500;//Hz
    fb=125;
    Fs=8000;//Hz

    nfft=pow(2,coarsefreqest_fft_power);
    fft = new FFT(nfft,false);
    ifft = new FFT(nfft,true);
    hzperbin=Fs/((double)nfft);
    out.resize(nfft);
    in.resize(nfft);
    y.resize(nfft);
    z.resize(nfft);
    startbin=round(lockingbw/hzperbin);
    stopbin=nfft-startbin;
    expectedpeakbin=round(fb/(2.0*hzperbin));
    emptyingcountdown=1;
}

void CoarseFreqEstimate::setSettings(int _coarsefreqest_fft_power,double _lockingbw,double _fb,double _Fs)
{
    coarsefreqest_fft_power=_coarsefreqest_fft_power;
    lockingbw=_lockingbw;//Hz
    fb=_fb;
    Fs=_Fs;//Hz
    delete ifft;
    delete fft;

    nfft=pow(2,coarsefreqest_fft_power);
    fft = new FFT(nfft,false);
    ifft = new FFT(nfft,true);
    hzperbin=Fs/((double)nfft);
    out.resize(nfft);
    in.resize(nfft);
    y.resize(nfft);
    z.resize(nfft);
    startbin=std::max(round(lockingbw/hzperbin),1.0);
    stopbin=nfft-startbin;
    expectedpeakbin=round(fb/(2.0*hzperbin));
}

CoarseFreqEstimate::~CoarseFreqEstimate()
{
    delete ifft;
    delete fft;
}

void CoarseFreqEstimate::bigchange()
{
    emptyingcountdown=4;
    for(int i=0;i<nfft;i++)y[i]=20;
}

void CoarseFreqEstimate::ProcessBasebandData(const QVector<cpx_type> &data)
{
    //fft size must be even. 2^n is best
    assert(nfft==data.size());

    //remove high frequencies then square and do fft and shift (0hz bin is at nfft/2)
    fft->transform(data.data(),out.data());
    for(int i=startbin;i<=stopbin;i++)out[i]=0;
    ifft->transform(out.data(),in.data());
    for(int i=0;i<nfft;i++)in[i]=in[i]*in[i];
    fft->transform(in.data(),out.data());
    for(int i=0;i<nfft/2;i++)std::swap(out[i+nfft/2],out[i]);


    //smooth
    for(int i=0;i<nfft;i++)y[i]=y[i]*0.9+0.1*10*log10(fmax(abs(out[i]),1));
    //for(int i=0;i<nfft;i++)y[i]=10*log10(fmax(abs(out[i]),1));

    //fold and look for the fold that produces a big peak at the expected peak location
    double zmax=0;
    int zmaxloc=nfft/2;
    assert(hzperbin!=0);
    for(int i=round((-lockingbw/hzperbin)+((double)(nfft/2)));i<round((lockingbw/hzperbin)+((double)(nfft/2)));i++)
    {
        if((i<0)||(i>=z.size()))continue;//jic
        double val=0;
        for(int j=-1;j<=1;j++)
        {
            if(((i-expectedpeakbin-j)<0)||((i+expectedpeakbin+j)>=y.size()))continue;//jic
            val+=(y[i-expectedpeakbin-j]+y[i+expectedpeakbin+j]);
        }
        z[i]=val;
        if(z[i]>zmax)
        {
            zmax=z[i];
            zmaxloc=i;
        }
    }
    freq_offset_est=-((double)(zmaxloc-nfft/2))*hzperbin*0.5;

    //emit the result to whoever wants it
    if(emptyingcountdown<=0)emit FreqOffsetEstimate(freq_offset_est);
     else {emptyingcountdown--;emit FreqOffsetEstimate(0);}

}

