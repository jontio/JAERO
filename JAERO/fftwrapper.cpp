#include "fftwrapper.h"
#include <assert.h>

template<class T>
FFTWrapper<T>::FFTWrapper(int nfft, bool inverse,bool kissfft_scaling)
{
    this->nfft=nfft;
    fft.init(nfft);
    this->inverse=inverse;
    this->kissfft_scaling=kissfft_scaling;
}

template<class T>
FFTWrapper<T>::~FFTWrapper()
{
}

template<class T>
void FFTWrapper<T>::transform(const QVector< std::complex<T> > &in, QVector< std::complex<T> > &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    for(int i=0;i<in.size();i++)
    {
        out[i]=in[i];
    }
    if(inverse)
    {
        //for inverse kiss fft doesn't match matlab ifft but the rest of JAERO expects the mucked up version of the inverse
        //so we have to scale the iff to match what is expected
        fft.ifft(out);
        if(kissfft_scaling)for(int i=0;i<out.size();i++)out[i]*=(double)out.size();
    }
    else fft.fft(out);
}

template class FFTWrapper<double>;
