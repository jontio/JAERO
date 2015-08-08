#ifndef FFTWRAPPER_H
#define FFTWRAPPER_H

#include <QVector>
#include <complex>
#include "../kiss_fft130/kiss_fft.h"

//underlying fft still uses the type in the  kiss_fft_type in the c stuff
template<typename T>
class FFTWrapper
{
public:
    FFTWrapper(int nfft,bool inverse);
    ~FFTWrapper();
    void transform(const QVector< std::complex<T> > &in, QVector< std::complex<T> > &out);
private:
    kiss_fft_cfg cfg;
    QVector<kiss_fft_cpx> privatein;
    QVector<kiss_fft_cpx> privateout;
    int nfft;
};

#endif // FFTWRAPPER_H
