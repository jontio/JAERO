#ifndef AUDIOOQPSKDEMODULATOR_H
#define AUDIOOQPSKDEMODULATOR_H

#include <QObject>
#include <QAudioInput>

#include "oqpskdemodulator.h"

class AudioOqpskDemodulator : public OqpskDemodulator
{
    Q_OBJECT
public:
    struct Settings : public OqpskDemodulator::Settings
    {
        QAudioDeviceInfo audio_device_in;
        double buffersizeinsecs;
        Settings()
        {
            audio_device_in=QAudioDeviceInfo::defaultInputDevice();
            buffersizeinsecs=1.0;
        }
    };
    explicit AudioOqpskDemodulator(QObject *parent = 0);
    ~AudioOqpskDemodulator();
    void start();
    void stop();
    void setSettings(Settings settings);
private:
    Settings settings;
    QAudioFormat m_format;
    QAudioInput *m_audioInput;
};

#endif // AUDIOOQPSKDEMODULATOR_H
