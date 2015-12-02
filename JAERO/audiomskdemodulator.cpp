#include "audiomskdemodulator.h"

#include <QDebug>

AudioMskDemodulator::AudioMskDemodulator(QObject *parent)
:   MskDemodulator(parent),
  m_audioInput(NULL)
{
//
}

void AudioMskDemodulator::start()
{
    MskDemodulator::start();
    if(m_audioInput)m_audioInput->start(this);
}

void AudioMskDemodulator::stop()
{
    if(m_audioInput)m_audioInput->stop();
    MskDemodulator::stop();
}

void AudioMskDemodulator::setSettings(Settings _settings)
{
    bool wasopen=isOpen();
    stop();

    //if Fs has changed or the audio device doesnt exist then need to redo the audio device
    if((_settings.Fs!=settings.Fs)||(!m_audioInput))
    {
        settings=_settings;

        if(m_audioInput)m_audioInput->deleteLater();

        //set the format
        m_format.setSampleRate(settings.Fs);
        m_format.setChannelCount(1);
        m_format.setSampleSize(16);
        m_format.setCodec("audio/pcm");
        m_format.setByteOrder(QAudioFormat::LittleEndian);
        m_format.setSampleType(QAudioFormat::SignedInt);

        //setup
        m_audioInput = new QAudioInput(settings.audio_device_in, m_format, this);
        m_audioInput->setBufferSize(settings.Fs);//1 second buffer
    }
    settings=_settings;

    MskDemodulator::setSettings(settings);

    if(wasopen)start();

}

AudioMskDemodulator::~AudioMskDemodulator()
{
    stop();
}

