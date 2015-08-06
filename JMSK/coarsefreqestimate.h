#ifndef COARSEFREQESTIMATE_H
#define COARSEFREQESTIMATE_H

#include <QObject>
#include <QVector>

#include "../kiss_fft130/kissfft.hh"

typedef kissfft<double> FFT;
typedef std::complex<double> cpx_type;

class CoarseFreqEstimate : public QObject
{
    Q_OBJECT
public:
    explicit CoarseFreqEstimate(QObject *parent = 0);
    ~CoarseFreqEstimate();
    void setSettings(int coarsefreqest_fft_power,double lockingbw,double fb,double Fs);
    void bigchange();
signals:
    void FreqOffsetEstimate(double freq_offset_est);
public slots:
    void ProcessBasebandData(const QVector<cpx_type> &data);
private:
    FFT *fft;
    FFT *ifft;
    QVector<cpx_type> out;
    QVector<cpx_type> in;
    QVector<double> y;
    QVector<double> z;
    double nfft;
    double Fs;
    int coarsefreqest_fft_power;
    double hzperbin;
    int startbin,stopbin;
    double lockingbw;
    double fb;
    int expectedpeakbin;
    double freq_offset_est;
    int emptyingcountdown;
};

#endif // COARSEFREQESTIMATE_H
