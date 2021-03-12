#include "fftrwrapper.h"
#include <assert.h>

template<class T>
FFTrWrapper<T>::FFTrWrapper(int nfft)//???
{
    this->nfft=nfft;
    fft.init(nfft);
}

template<class T>
FFTrWrapper<T>::~FFTrWrapper()
{

}

template<class T>
void FFTrWrapper<T>::transform(const QVector<T> &in, QVector< std::complex<T> > &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    fft.fft_real(const_cast < QVector<T> & > (in),out);
}

template<class T>
void FFTrWrapper<T>::transform(const QVector< std::complex<T> > &in, QVector<T> &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    fft.ifft_real(const_cast < QVector<std::complex<T>> & > (in),out);
}

template class FFTrWrapper<double>;
