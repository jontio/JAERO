#ifndef FFTWRAPPER_H
#define FFTWRAPPER_H

#include <QVector>
#include <complex>
#include "../../JFFT/jfft.h"

//underlying fft still uses the type in the  kiss_fft_type in the c stuff
template<typename T>
class FFTWrapper
{
public:
    FFTWrapper(int nfft, bool inverse, bool kissfft_scaling=true);
    ~FFTWrapper();
    void transform(const QVector< std::complex<T> > &in, QVector< std::complex<T> > &out);
private:
    JFFT fft;
    int nfft;
    bool inverse;
    bool kissfft_scaling;
};

#endif // FFTWRAPPER_H
