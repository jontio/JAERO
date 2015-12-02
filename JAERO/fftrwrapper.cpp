#include "fftrwrapper.h"
#include <assert.h>

template<class T>
FFTrWrapper<T>::FFTrWrapper(int _nfft,bool inverse)
{
    nfft=_nfft;
    cfg = kiss_fftr_alloc(nfft, inverse, NULL, NULL);
    privatescalar.resize(nfft);
    privatecomplex.resize(nfft);
}

template<class T>
FFTrWrapper<T>::~FFTrWrapper()
{
    free(cfg);

}

template<class T>
void FFTrWrapper<T>::transform(const QVector<T> &in, QVector< std::complex<T> > &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    for(int i=0;i<in.size();i++)
    {
        privatescalar[i]=in[i];
    }
    kiss_fftr(cfg,privatescalar.data(),privatecomplex.data());
    for(int i=0;i<out.size();i++)
    {
        out[i]=std::complex<T>((double)privatecomplex[i].r,(double)privatecomplex[i].i);
    }
}

template<class T>
void FFTrWrapper<T>::transform(const QVector< std::complex<T> > &in, QVector<T> &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    for(int i=0;i<in.size();i++)
    {
        privatecomplex[i].r=in[i].real();
        privatecomplex[i].i=in[i].imag();
    }
    kiss_fftri(cfg,privatecomplex.data(),privatescalar.data());
    for(int i=0;i<out.size();i++)
    {
        out[i]=privatescalar[i];
    }
}

template class FFTrWrapper<float>;
template class FFTrWrapper<double>;
