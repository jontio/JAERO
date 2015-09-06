#ifndef AUDIOMSKMODULATOR_H
#define AUDIOMSKMODULATOR_H

#include <QObject>
#include <QAudioOutput>

#include "mskmodulator.h"

class AudioMskModulator : public MskModulator
{
    Q_OBJECT
public:
    struct Settings : public MskModulator::Settings
    {
        QAudioDeviceInfo audio_device_out;
        Settings()
        {
            audio_device_out=QAudioDeviceInfo::defaultOutputDevice();
        }
    };
    explicit AudioMskModulator(QObject *parent = 0);
    ~AudioMskModulator();
    void setSettings(Settings settings);
    bool stopping;
signals:
    void statechanged(bool open);
    void closed();
    void opened();
public slots:
    void start();
    void stop();
    void stopgraceful();
    void startstop(bool on)
    {
        if(on)start();
         else stop();
    }

private:
    Settings settings;
    QAudioFormat m_format;
    QAudioOutput *m_audioOutput;
private slots:
    void stateChanged(QAudio::State state);
    void DelayedStateClosedSlot();
    void DelayedStateOpenSlot();
    void stopgracefulslot();
};





#endif // AUDIOMSKMODULATOR_H
