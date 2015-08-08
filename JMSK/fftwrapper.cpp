#include "fftwrapper.h"
#include <assert.h>

template<class T>
FFTWrapper<T>::FFTWrapper(int _nfft, bool inverse)
{
    nfft=_nfft;
    cfg = kiss_fft_alloc(nfft, inverse, NULL, NULL);
    privatein.resize(nfft);
    privateout.resize(nfft);
}

template<class T>
FFTWrapper<T>::~FFTWrapper()
{
    free(cfg);
}

template<class T>
void FFTWrapper<T>::transform(const QVector< std::complex<T> > &in, QVector< std::complex<T> > &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    for(int i=0;i<in.size();i++)
    {
        privatein[i].r=in[i].real();
        privatein[i].i=in[i].imag();
    }
    kiss_fft(cfg, privatein.data(), privateout.data());
    for(int i=0;i<out.size();i++)
    {
        out[i]=std::complex<T>((double)privateout[i].r,(double)privateout[i].i);
    }
}

template class FFTWrapper<float>;
template class FFTWrapper<double>;
