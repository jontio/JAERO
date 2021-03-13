#include "fftrwrapper.h"
#include <assert.h>

template<class T>
FFTrWrapper<T>::FFTrWrapper(int nfft, bool kissfft_scaling)
{
    this->kissfft_scaling=kissfft_scaling;
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

    //for forward kissfft sets the remaining conj complex to 0
    if(kissfft_scaling)for(int i=out.size()/2+1;i<out.size();i++)out[i]=0;
}

template<class T>
void FFTrWrapper<T>::transform(const QVector< std::complex<T> > &in, QVector<T> &out)
{
    assert(in.size()==out.size());
    assert(in.size()==nfft);
    fft.ifft_real(const_cast < QVector<std::complex<T>> & > (in),out);

    //for forward kissfft scales by nfft
    if(kissfft_scaling)for(int i=0;i<out.size();i++)out[i]*=(double)out.size();

}

template class FFTrWrapper<double>;
