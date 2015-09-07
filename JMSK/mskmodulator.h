#ifndef MSKMODULATOR_H
#define MSKMODULATOR_H

#include <QIODevice>
#include "DSP.h"

#include <QVector>
#include <complex>

#include <QPointer>

#include <QElapsedTimer>

#include <QObject>

class MskModulator : public QIODevice
{
    Q_OBJECT
public:
    struct Settings
    {
        double freq_center;
        double fb;
        double Fs;
        double secondsbeforereadysignalemited;
        Settings()
        {
            freq_center=1000;//Hz
            fb=125;//bps
            Fs=8000;//Hz
            secondsbeforereadysignalemited=0;
        }
    };
    explicit MskModulator(QObject *parent);
    ~MskModulator();

    void ConnectSourceDevice(QIODevice *datasourcedevice);
    void DisconnectSourceDevice();

    //this class is a device
    void start();
    void stop();
    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    void setSettings(Settings settings);
signals:
    void ReadyState();
private:
    WaveTable osc;
    WaveTable st;
    Settings settings;
    BaceConverter bc;

    QPointer<QIODevice> pdatasourcedevice;

    int bitcounter;
    int bitstosendbeforereadystatesignal;

};

#endif // MSKMODULATOR_H
