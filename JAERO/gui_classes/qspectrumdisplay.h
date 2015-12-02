#ifndef QSPECTRUMDISPLAY_H
#define QSPECTRUMDISPLAY_H

#include <QObject>
#include <complex>
#include <vector>

#include "../../qcustomplot/qcustomplot.h"
#include "fftrwrapper.h"

typedef FFTrWrapper<double> FFTr;
typedef std::complex<double> cpx_type;

#include <QMouseEvent>
#include <QTimerEvent>

#define SPECTRUM_FFT_POWER 13

class QSpectrumDisplay : public QCustomPlot
{
    Q_OBJECT
public:
    explicit QSpectrumDisplay(QWidget *parent = 0);
    ~QSpectrumDisplay();
signals:
    void CenterFreqChanged(double freq_center);
public slots:
    void setPlottables(double freq_est,double freq_center,double bandwidth);
    void setFFTData(const QVector<double> &data);
    void setSampleRate(double samplerate);
protected:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
private:
    QCPBars *freqmarker;
    QCPBars *lockingbwbar;
    FFTr *fftr;
    QVector<cpx_type> out;
    QVector<double> in;
    QVector<double> spec_log_abs_vals;
    QVector<double> spec_freq_vals;
    QVector<double> hann_window;
    double nfft;
    double Fs;
    QVector<double> freqmarker_x, freqmarker_y;
    QVector<double> lockingbwbar_x, lockingbwbar_y;
    double freq_center;
    double fb;
    double hzperbin;
    QElapsedTimer timer;
private slots:
};

#endif // QSPECTRUMDISPLAY_H
