#ifndef AUDIOBURSTOQPSKDEMODULATOR_H
#define AUDIOBURSTOQPSKDEMODULATOR_H

#include <QObject>
#include <QAudioInput>

#include "burstoqpskdemodulator.h"

class AudioBurstOqpskDemodulator : public BurstOqpskDemodulator
{
    Q_OBJECT
public:
    struct Settings : public BurstOqpskDemodulator::Settings
    {
        QAudioDeviceInfo audio_device_in;
        double buffersizeinsecs;
        Settings()
        {
            audio_device_in=QAudioDeviceInfo::defaultInputDevice();
            buffersizeinsecs=1.0;
        }
    };
    explicit AudioBurstOqpskDemodulator(QObject *parent = 0);
    ~AudioBurstOqpskDemodulator();
    void start();
    void stop();
    void setSettings(Settings settings);
    BurstOqpskDemodulator *demod2;
    void invalidatesettings(){demod2->invalidatesettings();BurstOqpskDemodulator::invalidatesettings();}
    void setSQL(bool state){demod2->setSQL(state);BurstOqpskDemodulator::setSQL(state);}
    void setAFC(bool state){demod2->setAFC(state);BurstOqpskDemodulator::setAFC(state);}
    void setScatterPointType(ScatterPointType type){demod2->setScatterPointType(type);BurstOqpskDemodulator::setScatterPointType(type);}
private:
    Settings settings;
    QAudioFormat m_format;
    QAudioInput *m_audioInput;


};

#endif // AUDIOBURSTOQPSKDEMODULATOR_H
