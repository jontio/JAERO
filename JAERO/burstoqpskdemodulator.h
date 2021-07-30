#ifndef BURSTOQPSKDEMODULATOR_H
#define BURSTOQPSKDEMODULATOR_H

#include <QIODevice>
#include "DSP.h"
#include <QVector>
#include <complex>
#include <QPointer>
#include <QObject>
#include <QElapsedTimer>
#include <assert.h>
#include "aerol.h"

#include "fftrwrapper.h"

typedef FFTrWrapper<double> FFTr;
typedef std::complex<double> cpx_type;

class BurstOqpskDemodulator : public QIODevice
{
    Q_OBJECT
public:
    enum ScatterPointType{ SPT_constellation, SPT_phaseoffseterror, SPT_phaseoffsetest, SPT_None};
    struct Settings
    {
        int coarsefreqest_fft_power;
        double freq_center;
        double lockingbw;
        double fb;
        double Fs;
        double signalthreshold;
        bool channel_stereo;
        bool zmqAudio;
        Settings()
        {
            coarsefreqest_fft_power=13;//2^coarsefreqest_fft_power
            freq_center=8000;//Hz
            lockingbw=10500;//Hz
            fb=10500;//bps
            Fs=48000;//Hz
            signalthreshold=0.6;
            channel_stereo=false;
            zmqAudio=false;
        }
    };
    explicit BurstOqpskDemodulator(QObject *parent);
    ~BurstOqpskDemodulator();
    void setAFC(bool state);
    void setSQL(bool state);
    void setCPUReduce(bool state);
    void setSettings(Settings settings);
    void invalidatesettings();
    void ConnectSinkDevice(QIODevice *datasinkdevice);
    void DisconnectSinkDevice();
    void start();
    void stop();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    double getCurrentFreq();
    void setScatterPointType(ScatterPointType type);

    //--L/R channel selection
    bool channel_select_other;

signals:
    void ScatterPoints(const QVector<cpx_type> &buffer);
    void OrgOverlapedBuffer(const QVector<double> &buffer);
    void PeakVolume(double Volume);
    void SampleRateChanged(double Fs);
    void BitRateChanged(double fb, bool burstmode);
    void Plottables(double freq_est,double freq_center,double bandwidth);
    void BBOverlapedBuffer(const QVector<cpx_type> &buffer);
    void MSESignal(double mse);
    void SignalStatus(bool gotasignal);
    void WarningTextSignal(const QString &str);
    void EbNoMeasurmentSignal(double EbNo);
    void writeDataSignal(const char *data, qint64 len);
    void processDemodulatedSoftBits(const QVector<short> &soft_bits);


private:

    const cpx_type imag=cpx_type(0, 1);

    QPointer<QIODevice> pdatasinkdevice;
    bool afc;
    bool sql;
    int scatterpointtype;

    double Fs;
    double freq_center;
    double lockingbw;
    double fb;
    double signalthreshold;

    double SamplesPerSymbol;
    bool insertpreamble;

    WaveTable mixer2;

    QVector<double> spectrumcycbuff;
    int spectrumcycbuff_ptr;
    int spectrumnfft;

    QVector<cpx_type> bbcycbuff;
    QVector<cpx_type> bbtmpbuff;
    int bbcycbuff_ptr;
    int bbnfft;

    QVector<cpx_type> pointbuff;
    int pointbuff_ptr;

    QElapsedTimer timer;

//--symbol timing detection
    AGC *agc;

    //hilbert
    QJHilbertFilter hfir;
    QVector<cpx_type> hfirbuff;

    //delay lines
    Delay< cpx_type > bt_d1;
    Delay< double > bt_ma_diff;

    //MAs
    TMovingAverage< std::complex<double> > bt_ma1;
    MovingAverage *mav1;


    //Peak detection
    PeakDetector pdet;

    //delay for peak detection alignment
    DelayThing< std::complex<double> > d1;

    //delay for trident detection
    DelayThing<double> d2;

    //trident shape thing
    QVector<double> tridentbuffer;
    int tridentbuffer_ptr;
    int tridentbuffer_sz;

    //fft for trident
    FFTr *fftr;
    QVector<cpx_type> out_base,out_top;
    QVector<double> out_abs_diff;
    QVector<double> in;

//--

//--demod

    AGC *agc2;


    FIR *fir_re;
    FIR *fir_im;

    //st
    Delay<double> delays;
    Delay<double> delayt41;
    Delay<double> delayt42;
    Delay<double> delayt8;
    IIR st_iir_resonator;
    WaveTable st_osc;
    WaveTable st_osc_ref;
    WaveTable st_osc_quarter;
    Delay<double> a1;
    double ee;
    cpx_type symboltone_averotator;
    double carrier_rotation_est;

    cpx_type rotator;
    double rotator_freq;

    //ct
    IIR ct_iir_loopfilter;


    OQPSKEbNoMeasure *ebnomeasure;

    BaceConverter bc;
    QByteArray  RxDataBytes;//packed in bytes
    QVector<short> RxDataBits;//unpacked soft bits



    double mse;
    MovingAverage *msema;

    QVector<cpx_type> phasepointbuff;
    int phasepointbuff_ptr;

//--




    DelayThing<cpx_type> rotation_bias_delay;
    MovingAverage *rotation_bias_ma;

    int startstopstart;

    //old static stuff
    cpx_type pt_d;
    int yui;
    cpx_type sig2_last;
    cpx_type symboltone_rotator;
    int startstop;
    double vol_gain;
    int cntr;
    double maxval;

    bool channel_stereo;

    bool cpuReduce;

   

public slots:
    void CenterFreqChangedSlot(double freq_center);
    void writeDataSlot(const char *data, qint64 len);
    void dataReceived(const QByteArray &audio, quint32 sampleRate);


};

#endif // BURSTOQPSKDEMODULATOR_H
