#include "fftwrapper.h"
#include <assert.h>

template<class T>
FFTWrapper<T>::FFTWrapper(int nfft, bool inverse)
{
    this->nfft=nfft;
    fft.init(nfft);
    this->inverse=inverse;
//    privatein.resize(nfft);
//    privateout.resize(nfft);
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
    if(inverse)fft.ifft(out);
    else fft.fft(out);
}

template class FFTWrapper<double>;
