#ifndef FFTRWRAPPER_H
#define FFTRWRAPPER_H

#include <QVector>
#include <complex>
#include "../kiss_fft130/kiss_fftr.h"

//underlying fft still uses the type in the  kiss_fft_type in the c stuff
template<typename T>
class FFTrWrapper
{
public:
    FFTrWrapper(int nfft,bool inverse);
    ~FFTrWrapper();
    void transform(const QVector<T> &in, QVector< std::complex<T> > &out);
    void transform(const QVector< std::complex<T> > &in, QVector<T> &out);
private:
    kiss_fftr_cfg cfg;
    QVector<kiss_fft_scalar> privatescalar;
    QVector<kiss_fft_cpx> privatecomplex;
    int nfft;
};

#endif // FFTRWRAPPER_H
