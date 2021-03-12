#ifndef FFTRWRAPPER_H
#define FFTRWRAPPER_H

#include <QVector>
#include <complex>
#include "../../JFFT/jfft.h"

template<typename T>
class FFTrWrapper
{
public:
    FFTrWrapper(int nfft);
    ~FFTrWrapper();
    void transform(const QVector<T> &in, QVector< std::complex<T> > &out);
    void transform(const QVector< std::complex<T> > &in, QVector<T> &out);
private:
    int nfft;
    JFFT fft;
};

#endif // FFTRWRAPPER_H
