#ifndef BURSTMSKDEMODULATOR_H
#define BURSTMSKDEMODULATOR_H

#include <QIODevice>
#include "DSP.h"

#include <QVector>
#include <complex>

#include <QPointer>

#include <QElapsedTimer>
#include "aerol.h"

#include "fftrwrapper.h"



typedef FFTrWrapper<double> FFTr;
typedef std::complex<double> cpx_type;

class CoarseFreqEstimate;


class BurstMskDemodulator : public QIODevice
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
        int symbolspercycle;
        double signalthreshold;
        Settings()
        {
            coarsefreqest_fft_power=13;//2^coarsefreqest_fft_power
            freq_center=1000;//Hz
            lockingbw=500;//Hz
            fb=125;//bps
            Fs=8000;//Hz
            symbolspercycle=16;
            signalthreshold=0.5;
        }
    };
    explicit BurstMskDemodulator(QObject *parent);
    ~BurstMskDemodulator();


    void ConnectSinkDevice(QIODevice *datasinkdevice);
    void DisconnectSinkDevice();


    void start();
    void stop();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    void setSettings(Settings settings);
    void invalidatesettings();
    void setAFC(bool state);
    void setSQL(bool state);
    void setScatterPointType(ScatterPointType type);
    double getCurrentFreq();
private:

    QString debug = "";

    WaveTable mixer_center;
    WaveTable mixer2;

    int spectrumnfft,bbnfft;

    QVector<cpx_type> bbcycbuff;
    QVector<cpx_type> bbtmpbuff;
    int bbcycbuff_ptr;

    QVector<double> spectrumcycbuff;
    QVector<double> spectrumtmpbuff;
    int spectrumcycbuff_ptr;

    CoarseFreqEstimate *coarsefreqestimate;

    double Fs;
    double freq_center;
    double lockingbw;
    double fb;
    double symbolspercycle;
    double signalthreshold;

    double SamplesPerSymbol;

    FIR *matchedfilter_re;
    FIR *matchedfilter_im;

    AGC *agc;
    AGC *agc2;


    MSKEbNoMeasure *ebnomeasure;

    MovingAverage *pointmean;

    QVector<cpx_type> sig2buff;
    int sig2buff_ptr;

    QVector<cpx_type> delaybuff;

    QVector<cpx_type> sigbuff;
    int sigbuff_ptr;

    int lastindex;

    QVector<cpx_type> pointbuff;
    int pointbuff_ptr;

    QVector<cpx_type> symbolbuff;
    int symbolbuff_ptr;

    SymTracker symtracker;

    QList<int> tixd;

    DiffDecode diffdecode;

    QVector<short> RxDataBits;//unpacked
    QByteArray  RxDataBytes;//packed in bytes

    double mse;

    MovingAverage *msema;

    bool afc;

    double symboltrackingweight;

    bool sql;

    int scatterpointtype;

    QVector<cpx_type> singlepointphasevector;

    BaceConverter bc;

    QPointer<QIODevice> pdatasinkdevice;

    QElapsedTimer timer;

    // trident detector stuff

    //hilbert
    QJHilbertFilter *hfir;
    QVector<kffsamp_t> hfirbuff;

    //delay lines
    Delay< std::complex<double> > bt_d1;
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

    int maxvalbin = 0;
    double trackfreq = 0;

    //fft for trident
    FFTr *fftr;
    QVector<cpx_type> out_base,out_top;
    QVector<double> out_abs_diff;
    QVector<double> in;
    double maxval;
    double vol_gain;
    int cntr;

    int startstopstart;
    int startstop;
    int trackingDelay;
    int numPoints;

    const cpx_type imag=cpx_type(0, 1);
    WaveTable st_osc;
    WaveTable st_osc_ref;
    WaveTable st_osc_quarter;

    Delay<double> a1;

    double ee;
    cpx_type symboltone_averotator;
    cpx_type symboltone_rotator;
    double carrier_rotation_est;
    cpx_type sig2_last;
    cpx_type pt_d;

    cpx_type rotator;
    double rotator_freq;

    bool even;
    double evenval;
    double oddval;

    //st
    Delay<double> delays;
    Delay<double> delayt41;
    Delay<double> delayt42;
    Delay<double> delayt8;
    IIR st_iir_resonator;
    int yui;

    int endRotation;



signals:
    void ScatterPoints(const QVector<cpx_type> &buffer);
    void SymbolPhase(double phase_rad);
    void BBOverlapedBuffer(const QVector<cpx_type> &buffer);
    void OrgOverlapedBuffer(const QVector<double> &buffer);
    void Plottables(double freq_est,double freq_center,double bandwidth);
    void PeakVolume(double Volume);
    void RxData(QByteArray &data);//packed in bytes
    void MSESignal(double mse);
    void SignalStatus(bool gotasignal);
    void WarningTextSignal(const QString &str);
    void EbNoMeasurmentSignal(double EbNo);
    void SampleRateChanged(double Fs);
    void BitRateChanged(double fb,bool burstmode);
    void processDemodulatedSoftBits(const QVector<short> &soft_bits);

public slots:
    void FreqOffsetEstimateSlot(double freq_offset_est);
    void CenterFreqChangedSlot(double freq_center);

};

#endif // BURSTMSKDEMODULATOR_H
